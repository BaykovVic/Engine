#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "sky/component/component_world.hpp"
#include "sky/ecs/ecs_world.hpp"
#include "sky/editor/viewport/play_mode_controller.hpp"
#include "sky/object/object_world.hpp"
#include "sky/physics/physics_world.hpp"
#include "sky/platform/platform_services.hpp"
#include "sky/platform/virtual_file_system.hpp"
#include "sky/scene/scene_world.hpp"
#include "sky/serialization/backends.hpp"

namespace sky::editor {

/// A serializable description of an object subtree: enough to delete an
/// object and bring it back identically (undo of delete).
struct ObjectSnapshot {
    std::string name;
    core::Transform local;
    bool hasPhysicsBody = false;
    std::vector<std::string> componentTypes;
    std::vector<ObjectSnapshot> children;
};

/// Everything the editor session works with: the assembled engine core plus
/// the demo scene. Qt-free; the UI layer consumes it through references.
class EditorContext {
public:
    EditorContext();

    object::ObjectHandle createEmpty(const std::string& name);
    /// A cube with a dynamic rigid body and box collider, Unity-style.
    object::ObjectHandle createCrate(const std::string& name, core::Vec3 position);
    void destroyObject(object::ObjectHandle object);

    /// Deep-copies an object with its components, physics binding and
    /// children; the copy becomes a sibling of the original.
    object::ObjectHandle duplicateObject(object::ObjectHandle object);

    /// Moves an object under a new parent (invalid parent = scene root),
    /// keeping the scene root list consistent.
    void reparent(object::ObjectHandle child, object::ObjectHandle newParent);

    [[nodiscard]] ObjectSnapshot snapshotObject(object::ObjectHandle object) const;
    /// Rebuilds an object subtree from a snapshot (invalid parent = root).
    object::ObjectHandle restoreObject(const ObjectSnapshot& snapshot,
                                       object::ObjectHandle parent);
    [[nodiscard]] bool hasPhysicsBody(object::ObjectHandle object) const {
        return bodies_.contains(object.value);
    }

    [[nodiscard]] std::vector<object::ObjectHandle> rootObjects() const {
        return roots_;
    }

    std::unique_ptr<platform::IFileSystem> fileSystem;
    std::unique_ptr<platform::IVirtualFileSystem> vfs;
    std::unique_ptr<serialization::ISerializationBackend> storage;
    std::unique_ptr<object::ObjectWorld> objects;
    std::unique_ptr<component::ComponentWorld> components;
    std::unique_ptr<ecs::EcsWorld> ecs;
    std::unique_ptr<physics::PhysicsWorld> physics;
    std::unique_ptr<physics::ObjectPhysicsSync> physicsSync;
    std::unique_ptr<scene::SceneWorld> scenes;
    std::unique_ptr<PlayModeController> playMode;
    scene::SceneHandle activeScene;

private:
    void buildDemoScene();
    object::ObjectHandle cloneSubtree(object::ObjectHandle source,
                                      object::ObjectHandle parent);
    void attachCrateBody(object::ObjectHandle object);

    std::vector<object::ObjectHandle> roots_;
    std::unordered_map<std::uint64_t, physics::RigidBodyHandle> bodies_;
};

} // namespace sky::editor
