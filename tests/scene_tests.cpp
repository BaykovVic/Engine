#include <filesystem>

#include "sky/component/component_world.hpp"
#include "sky/object/object_world.hpp"
#include "sky/platform/platform_services.hpp"
#include "sky/scene/scene_authoring.hpp"
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

void testSceneAuthoring() {
    SceneFixture fx;
    fx.components->registerComponentType(
        {"sky.mesh", "Mesh Renderer", false, "",
         {{"material", "string"}, {"mesh", "string"}}, "Rendering"});

    const sky::scene::AuthoringServices svc{*fx.objects,    *fx.objects,
                                            *fx.objects,    *fx.components,
                                            *fx.components, *fx.components};

    // createPrimitive: an object carrying a Mesh Renderer bound to the cube.
    const auto cube =
        sky::scene::createPrimitive(svc, sky::scene::PrimitiveKind::Cube, "Cube");
    CHECK(fx.objects->exists(cube));
    const auto comps = fx.components->componentsOf(cube);
    CHECK(comps.size() == 1u);
    const auto meshField = fx.components->field(comps.front(), "mesh");
    CHECK(meshField.has_value());
    CHECK(std::get<std::string>(*meshField) == "cube");

    // Give it a non-default field value and a child, then duplicate.
    fx.components->setField(comps.front(), "material", std::string("Crate"));
    const auto child = fx.objects->createObject("Child");
    fx.objects->setParent(child, cube);
    fx.objects->setLocalTransform(child, {{1.0f, 2.0f, 3.0f}, {}, {1, 1, 1}});

    const auto copy = sky::scene::duplicateObject(svc, cube);
    CHECK(fx.objects->exists(copy));
    CHECK(fx.objects->nameOf(copy) == "Cube Copy");

    // Component field values carry across (the faithful-copy guarantee).
    const auto copyComps = fx.components->componentsOf(copy);
    CHECK(copyComps.size() == 1u);
    const auto material = fx.components->field(copyComps.front(), "material");
    CHECK(material.has_value());
    CHECK(std::get<std::string>(*material) == "Crate");

    // The child is duplicated under the copy, keeping its name and transform.
    const auto copyChildren = fx.objects->childrenOf(copy);
    CHECK(copyChildren.size() == 1u);
    CHECK(fx.objects->nameOf(copyChildren.front()) == "Child");
    CHECK(fx.objects->localTransform(copyChildren.front()).position.y == 2.0f);
}

} // namespace

int main() {
    testSceneRoundTrip();
    testLoadRejectsCorruptScene();
    testSceneAuthoring();
    return sky::test::summary("scene_tests");
}
