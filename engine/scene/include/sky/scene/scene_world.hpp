#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "sky/component/component_world.hpp"
#include "sky/core/job_scheduler.hpp"
#include "sky/ecs/ecs.hpp"
#include "sky/ecs/object_sync.hpp"
#include "sky/object/object_model.hpp"
#include "sky/physics/physics.hpp"
#include "sky/scene/scene_system.hpp"
#include "sky/scripting/scripting_boundary.hpp"
#include "sky/serialization/backends.hpp"
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
    /// Explicit object <-> ECS sync, run around the system tick.
    ecs::IEcsObjectSync* ecsSync = nullptr;
    /// Component field persistence (scene schema >= 1.1).
    component::ComponentWorld* componentData = nullptr;
    /// Managed script callbacks, dispatched each frame after the systems.
    scripting::IScriptLifecycleBridge* scriptBridge = nullptr;
    /// When provided, the scene world registers its format migrations here
    /// and runs legacy files through them on load.
    serialization::SchemaMigrationService* migrations = nullptr;
    /// Optional asset-reference bridge (scene schema >= 1.2): string fields
    /// holding asset refs are persisted together with the asset GUID, so a
    /// renamed source (whose .skymeta travelled with it) still resolves.
    /// refToGuid maps a ref ("assets://…") to its GUID at save (0 = none);
    /// guidToRef maps a GUID back to the CURRENT ref at load ("" = unknown).
    std::function<std::uint64_t(const std::string&)> refToGuid;
    std::function<std::string(std::uint64_t)> guidToRef;
    /// Optional fixed-step script callback (Unity's FixedUpdate slot):
    /// invoked once before EVERY physics step with the fixed dt, so scripts
    /// see and influence each step, not just each frame. Only runs in
    /// synchronous stepping; while wantsFixedUpdate() returns true the scene
    /// keeps (or falls back to) sync stepping inside tick() even when a
    /// physics job scheduler is attached — callback code may touch any
    /// engine state, so it cannot run concurrently with the frame.
    std::function<void(double)> fixedUpdate;
    std::function<bool()> wantsFixedUpdate;
};

/// In-memory implementation of the Scene System: owns scene lifecycle, the
/// active scene context and object-to-scene membership.
class SceneWorld : public ISceneRepository, public ISceneRuntime, public ISceneQueryService {
public:
    ~SceneWorld() override = default;

    /// Registers an existing object (and its subtree) as a root of the scene.
    virtual void addRootObject(SceneHandle scene, object::ObjectHandle object) = 0;

    /// Replaces the scene's root list wholesale. The editor owns the live
    /// list (deletes and reparents mutate it there); save paths sync it here
    /// so the persisted scene never walks stale roots.
    virtual void setRootObjects(SceneHandle scene,
                                std::vector<object::ObjectHandle> roots) = 0;

    /// Saves a scene to an explicit path, optionally excluding one root subtree
    /// (e.g. an editor fixture not covered by the scene schema).
    virtual bool saveSceneAs(SceneHandle scene, const std::filesystem::path& path,
                             object::ObjectHandle excludeRoot) = 0;

    /// The current root objects of a scene (after load, to rebuild editor state).
    [[nodiscard]] virtual std::vector<object::ObjectHandle> rootObjectsOf(
        SceneHandle scene) const = 0;

    /// Moves the fixed-step physics onto the scheduler (Unigine-style async
    /// frame model): tick() applies the PREVIOUS frame's simulation results
    /// and runs systems against them; the caller then finishes ALL of its
    /// script work and calls schedulePhysics(dt), whose background steps
    /// overlap the render — at the cost of a one-frame lag. nullptr returns
    /// to synchronous stepping (schedulePhysics becomes a no-op). The
    /// pending step is always drained before results are read
    /// (deactivate/unload/scheduler swap).
    virtual void setPhysicsJobScheduler(core::IJobScheduler* scheduler) = 0;

    /// Async mode only: pushes the authored state and schedules this
    /// frame's fixed steps in the background. Call it strictly after every
    /// piece of code that may touch the physics world this frame — the job
    /// runs concurrently with everything that follows.
    virtual void schedulePhysics(double deltaSeconds) = 0;
};

std::unique_ptr<SceneWorld> createSceneWorld(const SceneWorldDeps& deps);

} // namespace sky::scene
