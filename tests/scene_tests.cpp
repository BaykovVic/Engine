#include <filesystem>

#include "sky/component/component_world.hpp"
#include "sky/object/object_world.hpp"
#include "sky/platform/platform_services.hpp"
#include "sky/scene/scene_world.hpp"
#include "sky/serialization/backends.hpp"
#include "sky_test.hpp"

namespace {

struct SceneFixture {
    std::unique_ptr<sky::object::ObjectWorld> objects =
        sky::object::createObjectWorld();
    std::unique_ptr<sky::component::ComponentWorld> components =
        sky::component::createComponentWorld();
    std::unique_ptr<sky::platform::IFileSystem> fileSystem =
        sky::platform::createStdFileSystem();
    std::unique_ptr<sky::serialization::ISerializationBackend> storage =
        sky::serialization::createFileSerializationBackend(*fileSystem);
    std::unique_ptr<sky::scene::SceneWorld> scenes = sky::scene::createSceneWorld({
        *objects, *objects, *objects, *components, *components, *storage});
};

void testSceneRoundTrip() {
    const auto path =
        std::filesystem::temp_directory_path() / "sky_engine_tests" / "main.scene";

    // Author a scene: root -> child hierarchy with components and transforms.
    {
        SceneFixture fx;
        fx.components->registerComponentType({"sky.mesh", "Mesh", false, "", {}});

        const auto scene = fx.scenes->createScene({"main", path});
        const auto root = fx.objects->createObject("level");
        const auto child = fx.objects->createObject("crate");
        fx.objects->setParent(child, root);
        fx.objects->setLocalTransform(child,
                                      {{1.0f, 2.0f, 3.0f}, {}, {1.0f, 1.0f, 1.0f}});
        fx.components->attach(child, "sky.mesh");
        fx.scenes->addRootObject(scene, root);

        CHECK(fx.scenes->saveScene(scene));
        CHECK(fx.scenes->sceneOf(child) == scene);
    }

    // Restore it into a fresh world and verify everything came back.
    {
        SceneFixture fx;
        fx.components->registerComponentType({"sky.mesh", "Mesh", false, "", {}});

        const auto scene = fx.scenes->loadScene(path);
        CHECK(scene.isValid());
        CHECK(fx.scenes->descriptor(scene).name == "main");

        const auto roots = fx.objects->findByName("level");
        CHECK(roots.size() == 1);
        const auto children = fx.objects->childrenOf(roots.front());
        CHECK(children.size() == 1);
        CHECK(fx.objects->nameOf(children.front()) == "crate");
        CHECK(fx.objects->localTransform(children.front()).position.y == 2.0f);
        CHECK(fx.components->componentsOf(children.front()).size() == 1);

        // Activate / deactivate the runtime context explicitly.
        fx.scenes->activate(scene);
        CHECK(fx.scenes->activeContext().state == sky::scene::SceneState::RuntimeActive);
        CHECK(fx.scenes->activeContext().rootObjects.size() == 1);
        fx.scenes->deactivate(scene);
        CHECK(fx.scenes->activeContext().state == sky::scene::SceneState::Loaded);

        // Unload destroys the scene's objects.
        const auto root = roots.front();
        fx.scenes->unloadScene(scene);
        CHECK(!fx.objects->exists(root));
        CHECK(fx.scenes->loadedScenes().empty());
    }

    std::filesystem::remove_all(std::filesystem::temp_directory_path() /
                                "sky_engine_tests");
}

void testLoadRejectsCorruptScene() {
    SceneFixture fx;
    const auto path =
        std::filesystem::temp_directory_path() / "sky_engine_tests" / "bad.scene";
    fx.fileSystem->writeAll(path, {std::byte{0xDE}, std::byte{0xAD}});
    CHECK(!fx.scenes->loadScene(path).isValid());
    std::filesystem::remove_all(std::filesystem::temp_directory_path() /
                                "sky_engine_tests");
}

} // namespace

int main() {
    testSceneRoundTrip();
    testLoadRejectsCorruptScene();
    return sky::test::summary("scene_tests");
}
