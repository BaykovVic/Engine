#pragma once

#include <memory>

#include "sky/core/math.hpp"
#include "sky/ecs/ecs_world.hpp"
#include "sky/object/object_model.hpp"

namespace sky::ecs {

/// Standard ECS component mirroring an object's local transform for
/// data-oriented systems.
struct EcsTransform {
    core::Transform value;
};

/// The explicit synchronization contract between the object world and the
/// ECS world (the open question from the architecture docs, resolved the
/// same way as physics):
///
/// - the Object Model owns authoring state; pushAuthoringState() projects
///   bound object transforms into EcsTransform components before systems run;
/// - ECS systems own data-oriented execution; pullEcsResults() writes the
///   resulting EcsTransform values back to the objects afterwards.
///
/// Neither world ever reaches into the other's storage: data crosses only
/// through this contract, so there is no implicit dual ownership.
class IEcsObjectSync {
public:
    virtual ~IEcsObjectSync() = default;

    /// Creates (or reuses) the entity bound to the object.
    virtual EntityId bind(object::ObjectHandle object) = 0;
    virtual void unbind(object::ObjectHandle object) = 0;
    [[nodiscard]] virtual EntityId entityOf(object::ObjectHandle object) const = 0;
    [[nodiscard]] virtual object::ObjectHandle objectOf(EntityId entity) const = 0;

    virtual void pushAuthoringState() = 0;
    virtual void pullEcsResults() = 0;
};

std::unique_ptr<IEcsObjectSync> createEcsObjectSync(
    EcsWorld& ecs, object::IObjectHierarchyAccess& hierarchy);

} // namespace sky::ecs
