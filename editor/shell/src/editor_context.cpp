#include "editor_context.hpp"

#include <filesystem>

namespace sky::editor {

EditorContext::EditorContext() {
    fileSystem = platform::createStdFileSystem();
    vfs = platform::createVirtualFileSystem();
    storage = serialization::createFileSerializationBackend(*fileSystem);
    objects = object::createObjectWorld();
    components = component::createComponentWorld();
    ecs = ecs::createEcsWorld();
    physics = physics::createPhysicsWorld();
    physicsSync = physics::createObjectPhysicsSync(*physics, *objects);
    scenes = scene::createSceneWorld({*objects, *objects, *objects, *components,
                                      *components, *storage, ecs.get(), physics.get(),
                                      physicsSync.get()});
    playMode = createPlayModeController(*scenes);

    // Standard mounts of an opened project: writable project root plus the
    // engine's shipped content.
    const auto projectRoot = std::filesystem::current_path();
    vfs->mount("project", platform::createDirectoryMount(*fileSystem, projectRoot), 10);
    vfs->mount("assets",
               platform::createDirectoryMount(*fileSystem, projectRoot / "docs", true),
               0);

    components->registerComponentType({"sky.mesh", "Mesh Renderer", false, "", {}});
    components->registerComponentType(
        {"sky.collider.box", "Box Collider", false, "", {{"halfExtents", "Vec3"}}});
    components->registerComponentType(
        {"sky.rigidbody", "Rigidbody", false, "", {{"mass", "float"}}});
    components->registerComponentType(
        {"sky.script", "Script", true, "Game.Behaviour", {}});

    buildDemoScene();
}

object::ObjectHandle EditorContext::createEmpty(const std::string& name) {
    const auto object = objects->createObject(name);
    scenes->addRootObject(activeScene, object);
    roots_.push_back(object);
    return object;
}

object::ObjectHandle EditorContext::createCrate(const std::string& name,
                                                core::Vec3 position) {
    const auto crate = createEmpty(name);
    objects->setLocalTransform(crate, {position, {}, {1.0f, 1.0f, 1.0f}});
    components->attach(crate, "sky.mesh");
    components->attach(crate, "sky.collider.box");
    components->attach(crate, "sky.rigidbody");

    const auto body = physics->createBody({physics::BodyType::Dynamic, 1.0f, {}});
    physics->attachCollider(body,
                            {physics::ColliderShape::Box, {0.5f, 0.5f, 0.5f}, 0.0f});
    physicsSync->bind(body, crate);
    bodies_.emplace(crate.value, body);
    return crate;
}

void EditorContext::destroyObject(object::ObjectHandle object) {
    if (const auto it = bodies_.find(object.value); it != bodies_.end()) {
        physicsSync->unbind(it->second);
        physics->destroyBody(it->second);
        bodies_.erase(it);
    }
    components->detachAllFrom(object);
    objects->destroyObject(object);
    std::erase(roots_, object);
}

void EditorContext::buildDemoScene() {
    activeScene = scenes->createScene({"SampleScene", {}});

    const auto ground = createEmpty("Ground");
    objects->setLocalTransform(ground, {{0.0f, -0.5f, 0.0f}, {}, {20.0f, 1.0f, 20.0f}});
    components->attach(ground, "sky.mesh");
    components->attach(ground, "sky.collider.box");
    const auto groundBody = physics->createBody(
        {physics::BodyType::Static, 0.0f, {{0.0f, -0.5f, 0.0f}, {}, {1, 1, 1}}});
    physics->attachCollider(groundBody,
                            {physics::ColliderShape::Box, {10.0f, 0.5f, 10.0f}, 0.0f});

    createCrate("Crate A", {-1.5f, 2.0f, 0.0f});
    createCrate("Crate B", {0.0f, 4.0f, 0.0f});
    createCrate("Crate C", {1.5f, 6.0f, 0.0f});

    const auto light = createEmpty("Directional Light");
    objects->setLocalTransform(light, {{0.0f, 8.0f, -5.0f}, {}, {1, 1, 1}});

    const auto camera = createEmpty("Main Camera");
    objects->setLocalTransform(camera, {{0.0f, 2.0f, -10.0f}, {}, {1, 1, 1}});
}

} // namespace sky::editor
