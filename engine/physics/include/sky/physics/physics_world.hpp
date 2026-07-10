#pragma once

#include <memory>

#include "sky/object/object_model.hpp"
#include "sky/physics/physics.hpp"

namespace sky::physics {

/// Concrete physics world. The implementation behind it is the engine's own
/// solver; the contract keeps it replaceable.
class PhysicsWorld : public IPhysicsWorld, public IPhysicsQueryService {
public:
    ~PhysicsWorld() override = default;

    virtual void setGravity(const core::Vec3& gravity) = 0;
    virtual void setBodyVelocity(RigidBodyHandle body, const core::Vec3& velocity) = 0;
    [[nodiscard]] virtual core::Vec3 bodyVelocity(RigidBodyHandle body) const = 0;
    virtual void setBodyTransform(RigidBodyHandle body, const core::Transform& transform) = 0;
};

std::unique_ptr<PhysicsWorld> createPhysicsWorld();

/// Implementation of the explicit physics <-> object world sync contract.
/// Bound pairs are synchronized in both directions around the simulation
/// step; neither side ever owns the other's state implicitly.
class ObjectPhysicsSync : public IPhysicsSyncContract {
public:
    ~ObjectPhysicsSync() override = default;

    virtual void bind(RigidBodyHandle body, object::ObjectHandle object) = 0;
    virtual void unbind(RigidBodyHandle body) = 0;
};

std::unique_ptr<ObjectPhysicsSync> createObjectPhysicsSync(
    PhysicsWorld& physics, object::IObjectHierarchyAccess& hierarchy);

} // namespace sky::physics
