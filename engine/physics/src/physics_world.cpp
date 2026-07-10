#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <utility>

#include "sky/physics/physics_world.hpp"

namespace sky::physics {
namespace {

struct BodyRecord {
    RigidBodyDesc desc;
    core::Transform transform;
    core::Vec3 velocity{};
    std::vector<ColliderHandle> colliders;
};

struct ColliderRecord {
    ColliderDesc desc;
    RigidBodyHandle body;
};

struct Aabb {
    core::Vec3 min;
    core::Vec3 max;

    [[nodiscard]] bool overlaps(const Aabb& other) const {
        return min.x < other.max.x && max.x > other.min.x &&
               min.y < other.max.y && max.y > other.min.y &&
               min.z < other.max.z && max.z > other.min.z;
    }
};

class PhysicsWorldImpl final : public PhysicsWorld {
public:
    // IPhysicsWorld

    RigidBodyHandle createBody(const RigidBodyDesc& desc) override {
        const RigidBodyHandle handle{nextId_++};
        bodies_.emplace(handle.value, BodyRecord{desc, desc.initialTransform});
        return handle;
    }

    void destroyBody(RigidBodyHandle body) override {
        const auto it = bodies_.find(body.value);
        if (it == bodies_.end()) {
            return;
        }
        for (const auto collider : it->second.colliders) {
            colliders_.erase(collider.value);
        }
        bodies_.erase(it);
    }

    ColliderHandle attachCollider(RigidBodyHandle body, const ColliderDesc& desc) override {
        const auto it = bodies_.find(body.value);
        if (it == bodies_.end()) {
            return ColliderHandle::invalid();
        }
        const ColliderHandle handle{nextId_++};
        colliders_.emplace(handle.value, ColliderRecord{desc, body});
        it->second.colliders.push_back(handle);
        return handle;
    }

    void detachCollider(ColliderHandle collider) override {
        const auto it = colliders_.find(collider.value);
        if (it == colliders_.end()) {
            return;
        }
        if (const auto bodyIt = bodies_.find(it->second.body.value);
            bodyIt != bodies_.end()) {
            std::erase(bodyIt->second.colliders, collider);
        }
        colliders_.erase(it);
    }

    void step(double fixedDeltaSeconds) override {
        const auto dt = static_cast<float>(fixedDeltaSeconds);

        for (auto& [id, body] : bodies_) {
            if (body.desc.type != BodyType::Dynamic) {
                continue;
            }
            body.velocity = body.velocity + gravity_ * dt;
            body.transform.position = body.transform.position + body.velocity * dt;
        }

        resolveHeightfields();
        detectAndResolve();
    }

    std::vector<CollisionEvent> drainCollisionEvents() override {
        return std::exchange(events_, {});
    }

    // IPhysicsQueryService

    std::optional<RaycastHit> raycast(const core::Vec3& origin, const core::Vec3& direction,
                                      float maxDistance) const override {
        std::optional<RaycastHit> best;
        for (const auto& [id, collider] : colliders_) {
            if (collider.desc.shape == ColliderShape::TerrainHeightfield) {
                float distance = 0.0f;
                core::Vec3 point{};
                core::Vec3 normal{};
                if (rayVsHeightfield(ColliderHandle{id}, collider, origin,
                                     direction, maxDistance, distance, point,
                                     normal) &&
                    (!best || distance < best->distance)) {
                    best = RaycastHit{ColliderHandle{id}, point, normal, distance};
                }
                continue;
            }
            const auto aabb = worldAabb(ColliderHandle{id}, collider);
            float distance = 0.0f;
            core::Vec3 normal{};
            if (!rayVsAabb(origin, direction, aabb, maxDistance, distance, normal)) {
                continue;
            }
            if (!best || distance < best->distance) {
                best = RaycastHit{ColliderHandle{id},
                                  origin + direction * distance, normal, distance};
            }
        }
        return best;
    }

    core::Transform bodyTransform(RigidBodyHandle body) const override {
        const auto it = bodies_.find(body.value);
        return it != bodies_.end() ? it->second.transform : core::Transform{};
    }

    // PhysicsWorld

    void setGravity(const core::Vec3& gravity) override { gravity_ = gravity; }

    void setBodyVelocity(RigidBodyHandle body, const core::Vec3& velocity) override {
        if (const auto it = bodies_.find(body.value); it != bodies_.end()) {
            it->second.velocity = velocity;
        }
    }

    core::Vec3 bodyVelocity(RigidBodyHandle body) const override {
        const auto it = bodies_.find(body.value);
        return it != bodies_.end() ? it->second.velocity : core::Vec3{};
    }

    void setBodyTransform(RigidBodyHandle body, const core::Transform& transform) override {
        if (const auto it = bodies_.find(body.value); it != bodies_.end()) {
            it->second.transform = transform;
        }
    }

private:
    Aabb worldAabb(ColliderHandle handle, const ColliderRecord& collider) const {
        const auto bodyIt = bodies_.find(collider.body.value);
        const auto center =
            bodyIt != bodies_.end() ? bodyIt->second.transform.position : core::Vec3{};
        // Spheres and capsules are bounded by their enclosing box; precise
        // narrow-phase shapes land behind the same contract later.
        core::Vec3 extents = collider.desc.halfExtents;
        if (collider.desc.shape == ColliderShape::Sphere ||
            collider.desc.shape == ColliderShape::Capsule) {
            extents = {collider.desc.radius, collider.desc.radius, collider.desc.radius};
        }
        return {{center.x - extents.x, center.y - extents.y, center.z - extents.z},
                {center.x + extents.x, center.y + extents.y, center.z + extents.z}};
    }

    /// Bilinear height sample of a heightfield collider at world (x, z),
    /// in the collider body's local frame.
    static float sampleHeightfield(const HeightfieldDesc& field, float x, float z) {
        const auto resolution = field.resolution;
        if (resolution < 2 || field.heights.size() <
                                  static_cast<std::size_t>(resolution) * resolution) {
            return 0.0f;
        }
        const float gx = std::clamp(x / field.scale.x, 0.0f,
                                    static_cast<float>(resolution - 1));
        const float gz = std::clamp(z / field.scale.z, 0.0f,
                                    static_cast<float>(resolution - 1));
        const auto x0 = static_cast<std::uint32_t>(gx);
        const auto z0 = static_cast<std::uint32_t>(gz);
        const auto x1 = std::min(x0 + 1, resolution - 1);
        const auto z1 = std::min(z0 + 1, resolution - 1);
        const float fx = gx - static_cast<float>(x0);
        const float fz = gz - static_cast<float>(z0);
        const auto sample = [&](std::uint32_t sx, std::uint32_t sz) {
            return field.heights[sz * resolution + sx];
        };
        const float top = sample(x0, z0) * (1.0f - fx) + sample(x1, z0) * fx;
        const float bottom = sample(x0, z1) * (1.0f - fx) + sample(x1, z1) * fx;
        return (top * (1.0f - fz) + bottom * fz) * field.scale.y;
    }

    /// Ray vs heightfield: fixed-step march to the first sample below the
    /// surface, then a short bisection refines the crossing. The normal comes
    /// from the heightfield gradient at the hit.
    bool rayVsHeightfield(ColliderHandle, const ColliderRecord& collider,
                          const core::Vec3& origin, const core::Vec3& direction,
                          float maxDistance, float& distance, core::Vec3& point,
                          core::Vec3& normal) const {
        const auto bodyIt = bodies_.find(collider.body.value);
        if (bodyIt == bodies_.end()) {
            return false;
        }
        const auto fieldOrigin = bodyIt->second.transform.position;
        const auto& field = collider.desc.heightfield;
        const auto heightAt = [&](const core::Vec3& p) {
            return fieldOrigin.y + sampleHeightfield(field, p.x - fieldOrigin.x,
                                                     p.z - fieldOrigin.z);
        };
        const auto above = [&](float t) {
            const core::Vec3 p = origin + direction * t;
            return p.y > heightAt(p);
        };
        if (!above(0.0f)) {
            return false; // starting under the surface: no crossing to report
        }
        const float step = std::max(0.05f, std::min(0.5f, maxDistance / 256.0f));
        float previous = 0.0f;
        for (float t = step; t <= maxDistance; t += step) {
            if (above(t)) {
                previous = t;
                continue;
            }
            // Crossed between `previous` and `t`: bisect to the surface.
            float low = previous;
            float high = t;
            for (int i = 0; i < 16; ++i) {
                const float mid = (low + high) * 0.5f;
                (above(mid) ? low : high) = mid;
            }
            distance = (low + high) * 0.5f;
            point = origin + direction * distance;
            point.y = heightAt(point);
            // Central differences of the surface height around the hit.
            const float d = 0.25f;
            const float hx1 = heightAt({point.x + d, 0.0f, point.z});
            const float hx0 = heightAt({point.x - d, 0.0f, point.z});
            const float hz1 = heightAt({point.x, 0.0f, point.z + d});
            const float hz0 = heightAt({point.x, 0.0f, point.z - d});
            core::Vec3 n{(hx0 - hx1) / (2.0f * d), 1.0f, (hz0 - hz1) / (2.0f * d)};
            const float length =
                std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
            normal = {n.x / length, n.y / length, n.z / length};
            return true;
        }
        return false;
    }

    /// Keeps dynamic bodies above every heightfield collider in the world.
    void resolveHeightfields() {
        for (const auto& [fieldId, fieldCollider] : colliders_) {
            if (fieldCollider.desc.shape != ColliderShape::TerrainHeightfield) {
                continue;
            }
            const auto fieldBodyIt = bodies_.find(fieldCollider.body.value);
            if (fieldBodyIt == bodies_.end()) {
                continue;
            }
            const auto fieldOrigin = fieldBodyIt->second.transform.position;

            for (auto& [bodyId, body] : bodies_) {
                if (body.desc.type != BodyType::Dynamic) {
                    continue;
                }
                // The dynamic body's lowest point: its first box/sphere
                // collider, or a half-unit fallback.
                float halfHeight = 0.5f;
                for (const auto collider : body.colliders) {
                    const auto& desc = colliders_.at(collider.value).desc;
                    halfHeight = desc.shape == ColliderShape::Sphere ? desc.radius
                                                                     : desc.halfExtents.y;
                    break;
                }
                const float localX = body.transform.position.x - fieldOrigin.x;
                const float localZ = body.transform.position.z - fieldOrigin.z;
                const float ground =
                    fieldOrigin.y +
                    sampleHeightfield(fieldCollider.desc.heightfield, localX, localZ);
                const float bottom = body.transform.position.y - halfHeight;
                if (bottom < ground) {
                    body.transform.position.y = ground + halfHeight;
                    if (body.velocity.y < 0.0f) {
                        body.velocity.y = 0.0f;
                    }
                    events_.push_back({ColliderHandle{fieldId},
                                       body.colliders.empty()
                                           ? ColliderHandle::invalid()
                                           : body.colliders.front()});
                }
            }
        }
    }

    void detectAndResolve() {
        std::vector<std::pair<std::uint64_t, Aabb>> boxes;
        boxes.reserve(colliders_.size());
        for (const auto& [id, collider] : colliders_) {
            // Heightfields are handled by resolveHeightfields(), not by the
            // AABB pass.
            if (collider.desc.shape == ColliderShape::TerrainHeightfield) {
                continue;
            }
            boxes.emplace_back(id, worldAabb(ColliderHandle{id}, collider));
        }

        for (std::size_t i = 0; i < boxes.size(); ++i) {
            for (std::size_t j = i + 1; j < boxes.size(); ++j) {
                const auto& a = colliders_.at(boxes[i].first);
                const auto& b = colliders_.at(boxes[j].first);
                if (a.body == b.body || !boxes[i].second.overlaps(boxes[j].second)) {
                    continue;
                }
                events_.push_back(
                    {ColliderHandle{boxes[i].first}, ColliderHandle{boxes[j].first}});
                resolve(a, boxes[i].second, b, boxes[j].second);
                // Resolution moved a body; refresh its box for later pairs.
                boxes[i].second = worldAabb(ColliderHandle{boxes[i].first}, a);
                boxes[j].second = worldAabb(ColliderHandle{boxes[j].first}, b);
            }
        }
    }

    void resolve(const ColliderRecord& a, const Aabb& boxA, const ColliderRecord& b,
                 const Aabb& boxB) {
        auto& bodyA = bodies_.at(a.body.value);
        auto& bodyB = bodies_.at(b.body.value);

        // Only dynamic-vs-(static|kinematic) is positionally resolved for
        // now; dynamic pairs just report the contact.
        BodyRecord* dynamicBody = nullptr;
        const Aabb* dynamicBox = nullptr;
        const Aabb* otherBox = nullptr;
        if (bodyA.desc.type == BodyType::Dynamic && bodyB.desc.type != BodyType::Dynamic) {
            dynamicBody = &bodyA;
            dynamicBox = &boxA;
            otherBox = &boxB;
        } else if (bodyB.desc.type == BodyType::Dynamic &&
                   bodyA.desc.type != BodyType::Dynamic) {
            dynamicBody = &bodyB;
            dynamicBox = &boxB;
            otherBox = &boxA;
        } else {
            return;
        }

        // Push out along the axis of least penetration and kill the velocity
        // component pointing into the contact.
        const float penX = std::min(dynamicBox->max.x - otherBox->min.x,
                                    otherBox->max.x - dynamicBox->min.x);
        const float penY = std::min(dynamicBox->max.y - otherBox->min.y,
                                    otherBox->max.y - dynamicBox->min.y);
        const float penZ = std::min(dynamicBox->max.z - otherBox->min.z,
                                    otherBox->max.z - dynamicBox->min.z);

        auto& pos = dynamicBody->transform.position;
        auto& vel = dynamicBody->velocity;
        const auto centerOf = [](const Aabb& box, int axis) {
            switch (axis) {
                case 0: return (box.min.x + box.max.x) * 0.5f;
                case 1: return (box.min.y + box.max.y) * 0.5f;
                default: return (box.min.z + box.max.z) * 0.5f;
            }
        };

        if (penX <= penY && penX <= penZ) {
            const float sign = centerOf(*dynamicBox, 0) < centerOf(*otherBox, 0) ? -1.0f : 1.0f;
            pos.x += sign * penX;
            vel.x = 0.0f;
        } else if (penY <= penZ) {
            const float sign = centerOf(*dynamicBox, 1) < centerOf(*otherBox, 1) ? -1.0f : 1.0f;
            pos.y += sign * penY;
            vel.y = 0.0f;
        } else {
            const float sign = centerOf(*dynamicBox, 2) < centerOf(*otherBox, 2) ? -1.0f : 1.0f;
            pos.z += sign * penZ;
            vel.z = 0.0f;
        }
    }

    static bool rayVsAabb(const core::Vec3& origin, const core::Vec3& direction,
                          const Aabb& box, float maxDistance, float& outDistance,
                          core::Vec3& outNormal) {
        float tMin = 0.0f;
        float tMax = maxDistance;
        core::Vec3 normal{};

        const float originAxis[3] = {origin.x, origin.y, origin.z};
        const float dirAxis[3] = {direction.x, direction.y, direction.z};
        const float minAxis[3] = {box.min.x, box.min.y, box.min.z};
        const float maxAxis[3] = {box.max.x, box.max.y, box.max.z};

        for (int axis = 0; axis < 3; ++axis) {
            if (std::fabs(dirAxis[axis]) < 1e-8f) {
                if (originAxis[axis] < minAxis[axis] || originAxis[axis] > maxAxis[axis]) {
                    return false;
                }
                continue;
            }
            const float inv = 1.0f / dirAxis[axis];
            float t1 = (minAxis[axis] - originAxis[axis]) * inv;
            float t2 = (maxAxis[axis] - originAxis[axis]) * inv;
            float sign = -1.0f;
            if (t1 > t2) {
                std::swap(t1, t2);
                sign = 1.0f;
            }
            if (t1 > tMin) {
                tMin = t1;
                normal = {};
                if (axis == 0) normal.x = sign;
                if (axis == 1) normal.y = sign;
                if (axis == 2) normal.z = sign;
            }
            tMax = std::min(tMax, t2);
            if (tMin > tMax) {
                return false;
            }
        }

        outDistance = tMin;
        outNormal = normal;
        return true;
    }

    std::uint64_t nextId_ = 1;
    core::Vec3 gravity_{0.0f, -9.81f, 0.0f};
    std::unordered_map<std::uint64_t, BodyRecord> bodies_;
    std::unordered_map<std::uint64_t, ColliderRecord> colliders_;
    std::vector<CollisionEvent> events_;
};

class ObjectPhysicsSyncImpl final : public ObjectPhysicsSync {
public:
    ObjectPhysicsSyncImpl(PhysicsWorld& physics, object::IObjectHierarchyAccess& hierarchy)
        : physics_(physics), hierarchy_(hierarchy) {}

    void bind(RigidBodyHandle body, object::ObjectHandle object) override {
        bindings_[body.value] = object;
    }

    void unbind(RigidBodyHandle body) override { bindings_.erase(body.value); }

    void pushKinematicState() override {
        // Authoring moves objects; physics follows before the step.
        for (const auto& [bodyId, object] : bindings_) {
            physics_.setBodyTransform(RigidBodyHandle{bodyId},
                                      hierarchy_.worldTransform(object));
        }
    }

    void pullSimulationResults() override {
        // After the step the simulation owns dynamic motion; objects follow.
        // Body poses are world-space (push sends world transforms), so the
        // write-back goes through world space too: a parented object gets
        // its local transform via invCompose instead of the raw body pose.
        for (const auto& [bodyId, object] : bindings_) {
            object::setWorldTransform(
                hierarchy_, object,
                physics_.bodyTransform(RigidBodyHandle{bodyId}));
        }
    }

private:
    PhysicsWorld& physics_;
    object::IObjectHierarchyAccess& hierarchy_;
    std::unordered_map<std::uint64_t, object::ObjectHandle> bindings_;
};

} // namespace

std::unique_ptr<PhysicsWorld> createPhysicsWorld() {
    return std::make_unique<PhysicsWorldImpl>();
}

std::unique_ptr<ObjectPhysicsSync> createObjectPhysicsSync(
    PhysicsWorld& physics, object::IObjectHierarchyAccess& hierarchy) {
    return std::make_unique<ObjectPhysicsSyncImpl>(physics, hierarchy);
}

} // namespace sky::physics
