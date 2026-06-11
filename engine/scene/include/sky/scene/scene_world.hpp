#pragma once

#include <memory>

#include "sky/component/component_model.hpp"
#include "sky/ecs/ecs.hpp"
#include "sky/object/object_model.hpp"
#include "sky/physics/physics.hpp"
#include "sky/scene/scene_system.hpp"
#include "sky/serialization/serialization.hpp"

namespace sky::scene {

/// Everything the scene world needs from neighbouring modules. The scene
/// system orchestrates these; it never reaches into their internals.
/// The runtime subsystems are optional: without them tick() only maintains
/// scene state.
struct SceneWorldDeps {
    object::IObjectFactory& objectFactory;
    object::IObjectHierarchyAccess& hierarchy;
    object::IObjectQueryService& objectQuery;
    component::IComponentAttachmentService& componentAttachment;
    component::IComponentQueryService& componentQuery;
    serialization::ISerializationBackend& storage;
    ecs::IEcsSystemScheduler* ecsScheduler = nullptr;
    physics::IPhysicsWorld* physicsWorld = nullptr;
    physics::IPhysicsSyncContract* physicsSync = nullptr;
};

/// In-memory implementation of the Scene System: owns scene lifecycle, the
/// active scene context and object-to-scene membership.
class SceneWorld : public ISceneRepository, public ISceneRuntime, public ISceneQueryService {
public:
    ~SceneWorld() override = default;

    /// Registers an existing object (and its subtree) as a root of the scene.
    virtual void addRootObject(SceneHandle scene, object::ObjectHandle object) = 0;
};

std::unique_ptr<SceneWorld> createSceneWorld(const SceneWorldDeps& deps);

} // namespace sky::scene
