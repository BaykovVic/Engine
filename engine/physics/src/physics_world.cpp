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

    void detectAndResolve() {
        std::vector<std::pair<std::uint64_t, Aabb>> boxes;
        boxes.reserve(colliders_.size());
        for (const auto& [id, collider] : colliders_) {
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
        for (const auto& [bodyId, object] : bindings_) {
            hierarchy_.setLocalTransform(object,
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
