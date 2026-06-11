#pragma once

#include <optional>
#include <vector>

#include "sky/core/handle.hpp"
#include "sky/core/math.hpp"

namespace sky::physics {

struct RigidBodyTag {};
struct ColliderTag {};
using RigidBodyHandle = core::Handle<RigidBodyTag>;
using ColliderHandle = core::Handle<ColliderTag>;

enum class BodyType {
    Static,
    Kinematic,
    Dynamic,
};

struct RigidBodyDesc {
    BodyType type = BodyType::Dynamic;
    float mass = 1.0f;
    core::Transform initialTransform;
};

enum class ColliderShape {
    Box,
    Sphere,
    Capsule,
    /// Terrain collision is rebuilt from Terrain and Landscape data.
    TerrainHeightfield,
};

struct ColliderDesc {
    ColliderShape shape = ColliderShape::Box;
    core::Vec3 halfExtents{0.5f, 0.5f, 0.5f};
    float radius = 0.5f;
};

struct RaycastHit {
    ColliderHandle collider;
    core::Vec3 point;
    core::Vec3 normal;
    float distance = 0.0f;
};

struct CollisionEvent {
    ColliderHandle first;
    ColliderHandle second;
};

/// Physics Engine contract: the physics world. The backend behind this
/// contract is replaceable by design.
class IPhysicsWorld {
public:
    virtual ~IPhysicsWorld() = default;

    virtual RigidBodyHandle createBody(const RigidBodyDesc& desc) = 0;
    virtual void destroyBody(RigidBodyHandle body) = 0;
    virtual ColliderHandle attachCollider(RigidBodyHandle body, const ColliderDesc& desc) = 0;
    virtual void detachCollider(ColliderHandle collider) = 0;

    virtual void step(double fixedDeltaSeconds) = 0;
    [[nodiscard]] virtual std::vector<CollisionEvent> drainCollisionEvents() = 0;
};

/// Physics Engine contract: queries against the simulated world.
class IPhysicsQueryService {
public:
    virtual ~IPhysicsQueryService() = default;

    [[nodiscard]] virtual std::optional<RaycastHit> raycast(const core::Vec3& origin,
                                                            const core::Vec3& direction,
                                                            float maxDistance) const = 0;
    [[nodiscard]] virtual core::Transform bodyTransform(RigidBodyHandle body) const = 0;
};

/// Physics Engine contract: the explicit synchronization boundary between
/// the physics world and the scene/object/ECS worlds. No implicit dual
/// ownership: transforms are pushed in before the step and pulled out after.
class IPhysicsSyncContract {
public:
    virtual ~IPhysicsSyncContract() = default;

    virtual void pushKinematicState() = 0;
    virtual void pullSimulationResults() = 0;
};

} // namespace sky::physics
