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

/// Everything the editor session works with: the assembled engine core plus
/// the demo scene. Qt-free; the UI layer consumes it through references.
class EditorContext {
public:
    EditorContext();

    object::ObjectHandle createEmpty(const std::string& name);
    /// A cube with a dynamic rigid body and box collider, Unity-style.
    object::ObjectHandle createCrate(const std::string& name, core::Vec3 position);
    void destroyObject(object::ObjectHandle object);

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

    std::vector<object::ObjectHandle> roots_;
    std::unordered_map<std::uint64_t, physics::RigidBodyHandle> bodies_;
};

} // namespace sky::editor
