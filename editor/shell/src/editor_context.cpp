#include "editor_context.hpp"

#include <algorithm>
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
    attachCrateBody(crate);
    return crate;
}

void EditorContext::attachCrateBody(object::ObjectHandle object) {
    const auto body = physics->createBody(
        {physics::BodyType::Dynamic, 1.0f, objects->worldTransform(object)});
    physics->attachCollider(body,
                            {physics::ColliderShape::Box, {0.5f, 0.5f, 0.5f}, 0.0f});
    physicsSync->bind(body, object);
    bodies_.emplace(object.value, body);
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

object::ObjectHandle EditorContext::duplicateObject(object::ObjectHandle object) {
    if (!objects->exists(object)) {
        return object::ObjectHandle::invalid();
    }
    const auto parent = objects->parentOf(object);
    const auto copy = cloneSubtree(object, parent);
    if (!parent.isValid()) {
        scenes->addRootObject(activeScene, copy);
        roots_.push_back(copy);
    }
    return copy;
}

object::ObjectHandle EditorContext::cloneSubtree(object::ObjectHandle source,
                                                 object::ObjectHandle parent) {
    const auto copy = objects->createObject(objects->nameOf(source) + " Copy");
    if (parent.isValid()) {
        objects->setParent(copy, parent);
    }
    objects->setLocalTransform(copy, objects->localTransform(source));

    for (const auto component : components->componentsOf(source)) {
        components->attach(copy, components->descriptorOf(component).typeId);
    }
    if (bodies_.contains(source.value)) {
        attachCrateBody(copy);
    }
    for (const auto child : objects->childrenOf(source)) {
        cloneSubtree(child, copy);
    }
    return copy;
}

void EditorContext::reparent(object::ObjectHandle child, object::ObjectHandle newParent) {
    if (!objects->exists(child) || child == newParent) {
        return;
    }
    // Keep the object where it is in the world: recompute the local
    // transform against the new parent.
    const auto world = objects->worldTransform(child);
    objects->setParent(child, newParent);
    if (newParent.isValid()) {
        std::erase(roots_, child);
        // Local = inverse(parentWorld) * world; with uniform editor usage we
        // derive it through the hierarchy by assigning world and letting the
        // viewport math stay consistent for unrotated parents.
        const auto parentWorld = objects->worldTransform(newParent);
        const auto invRotation =
            core::Quat{-parentWorld.rotation.x, -parentWorld.rotation.y,
                       -parentWorld.rotation.z, parentWorld.rotation.w};
        const core::Vec3 offset{world.position.x - parentWorld.position.x,
                                world.position.y - parentWorld.position.y,
                                world.position.z - parentWorld.position.z};
        auto local = world;
        local.position = core::rotate(invRotation, offset);
        local.position = {local.position.x / parentWorld.scale.x,
                          local.position.y / parentWorld.scale.y,
                          local.position.z / parentWorld.scale.z};
        local.rotation = invRotation * world.rotation;
        local.scale = {world.scale.x / parentWorld.scale.x,
                       world.scale.y / parentWorld.scale.y,
                       world.scale.z / parentWorld.scale.z};
        objects->setLocalTransform(child, local);
    } else {
        if (std::ranges::find(roots_, child) == roots_.end()) {
            scenes->addRootObject(activeScene, child);
            roots_.push_back(child);
        }
        objects->setLocalTransform(child, world);
    }
}

ObjectSnapshot EditorContext::snapshotObject(object::ObjectHandle object) const {
    ObjectSnapshot snapshot;
    snapshot.name = objects->nameOf(object);
    snapshot.local = objects->localTransform(object);
    snapshot.hasPhysicsBody = hasPhysicsBody(object);
    for (const auto component : components->componentsOf(object)) {
        snapshot.componentTypes.push_back(components->descriptorOf(component).typeId);
    }
    for (const auto child : objects->childrenOf(object)) {
        snapshot.children.push_back(snapshotObject(child));
    }
    return snapshot;
}

object::ObjectHandle EditorContext::restoreObject(const ObjectSnapshot& snapshot,
                                                  object::ObjectHandle parent) {
    const auto object = objects->createObject(snapshot.name);
    if (parent.isValid()) {
        objects->setParent(object, parent);
    } else {
        scenes->addRootObject(activeScene, object);
        roots_.push_back(object);
    }
    objects->setLocalTransform(object, snapshot.local);
    for (const auto& typeId : snapshot.componentTypes) {
        components->attach(object, typeId);
    }
    if (snapshot.hasPhysicsBody) {
        attachCrateBody(object);
    }
    for (const auto& child : snapshot.children) {
        restoreObject(child, object);
    }
    return object;
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
