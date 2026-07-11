#include <filesystem>

#include "sky/component/component_world.hpp"
#include "sky/object/object_world.hpp"
#include "sky/platform/platform_services.hpp"
#include "sky/scene/scene_authoring.hpp"
#include "sky/scene/scene_world.hpp"
#include "sky/serialization/backends.hpp"
#include "sky/serialization/byte_stream.hpp"
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
        std::filesystem::temp_directory_path() / "sky_engine_tests" / "scene" / "main.scene";

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
                                "sky_engine_tests" / "scene");
}

void testGuidReferenceResolution() {
    const auto path = std::filesystem::temp_directory_path() /
                      "sky_engine_tests" / "scene" / "refs.scene";

    // Save a scene whose mesh field is an asset ref; the bridge reports its
    // GUID as 42 at save time.
    {
        SceneFixture fx;
        fx.components->registerComponentType(
            {"sky.mesh", "Mesh", false, "", {{"mesh", "string"}}});
        sky::scene::SceneWorldDeps deps{*fx.objects,    *fx.objects,
                                        *fx.objects,    *fx.components,
                                        *fx.components, *fx.storage};
        deps.componentData = fx.components.get();
        deps.refToGuid = [](const std::string& ref) -> std::uint64_t {
            return ref == "assets://Models/old.obj" ? 42u : 0u;
        };
        const auto scenes = sky::scene::createSceneWorld(deps);

        const auto scene = scenes->createScene({"refs", path});
        const auto object = fx.objects->createObject("model");
        const auto mesh = fx.components->attach(object, "sky.mesh");
        fx.components->setField(mesh, "mesh",
                                std::string("assets://Models/old.obj"));
        scenes->addRootObject(scene, object);
        CHECK(scenes->saveScene(scene));
    }

    // Load in a fresh world where the asset has moved: GUID 42 now resolves
    // to a different ref, and the field follows it.
    {
        SceneFixture fx;
        fx.components->registerComponentType(
            {"sky.mesh", "Mesh", false, "", {{"mesh", "string"}}});
        sky::scene::SceneWorldDeps deps{*fx.objects,    *fx.objects,
                                        *fx.objects,    *fx.components,
                                        *fx.components, *fx.storage};
        deps.componentData = fx.components.get();
        deps.guidToRef = [](std::uint64_t guid) -> std::string {
            return guid == 42u ? "assets://Models/renamed.obj" : "";
        };
        const auto scenes = sky::scene::createSceneWorld(deps);

        const auto scene = scenes->loadScene(path);
        CHECK(scene.isValid());
        const auto models = fx.objects->findByName("model");
        CHECK(models.size() == 1);
        if (models.empty()) {
            return; // the checks above already recorded the failure
        }
        const auto meshes = fx.components->componentsOf(models.front());
        CHECK(!meshes.empty());
        if (meshes.empty()) {
            return;
        }
        const auto mesh = meshes.front();
        const auto value = fx.components->field(mesh, "mesh");
        CHECK(value.has_value());
        const auto* text = std::get_if<std::string>(&*value);
        CHECK(text != nullptr && *text == "assets://Models/renamed.obj");
    }

    std::filesystem::remove_all(std::filesystem::temp_directory_path() /
                                "sky_engine_tests" / "scene");
}

void testLegacyV11SceneMigrates() {
    const auto path = std::filesystem::temp_directory_path() /
                      "sky_engine_tests" / "scene" / "legacy11.scene";

    // A hand-written 1.1 payload: one object, one component, one field of
    // every tag — the 1.1 -> 1.2 migration must walk all of them.
    {
        SceneFixture fx;
        sky::serialization::ByteWriter writer;
        writer.writeString("legacy");
        writer.writeU32(1); // objects
        writer.writeU32(0xFFFFFFFFu); // no parent
        writer.writeString("relic");
        for (int f = 0; f < 10; ++f) {
            writer.writeF32(f == 7 ? 1.0f : 0.0f); // identity-ish transform
        }
        writer.writeU32(1); // components
        writer.writeString("sky.mesh");
        writer.writeU32(5); // fields
        writer.writeString("f");
        writer.writeU32(0); // Float
        writer.writeF32(2.5f);
        writer.writeString("i");
        writer.writeU32(1); // Int
        writer.writeU64(7);
        writer.writeString("b");
        writer.writeU32(2); // Bool
        writer.writeU32(1);
        writer.writeString("mesh");
        writer.writeU32(3); // String — 1.1 stores no GUID after it
        writer.writeString("assets://Models/relic.obj");
        writer.writeString("v");
        writer.writeU32(4); // Vec3
        writer.writeF32(1.0f);
        writer.writeF32(2.0f);
        writer.writeF32(3.0f);
        CHECK(fx.storage->write(path, {"sky.scene", {1, 1}, writer.takeBuffer()}));
    }

    // Loading runs the 1.1 -> 1.2 migration chain; every field survives.
    {
        SceneFixture fx;
        fx.components->registerComponentType(
            {"sky.mesh", "Mesh", false, "", {{"mesh", "string"}}});
        const auto migrations =
            sky::serialization::createSchemaMigrationService();
        sky::scene::SceneWorldDeps deps{*fx.objects,    *fx.objects,
                                        *fx.objects,    *fx.components,
                                        *fx.components, *fx.storage};
        deps.componentData = fx.components.get();
        deps.migrations = migrations.get();
        const auto scenes = sky::scene::createSceneWorld(deps);

        const auto scene = scenes->loadScene(path);
        CHECK(scene.isValid());
        const auto relics = fx.objects->findByName("relic");
        CHECK(relics.size() == 1);
        if (relics.empty()) {
            return; // the checks above already recorded the failure
        }
        const auto meshes = fx.components->componentsOf(relics.front());
        CHECK(!meshes.empty());
        if (meshes.empty()) {
            return;
        }
        const auto mesh = meshes.front();
        const auto ref = fx.components->field(mesh, "mesh");
        CHECK(ref.has_value());
        const auto* text = std::get_if<std::string>(&*ref);
        CHECK(text != nullptr && *text == "assets://Models/relic.obj");
        const auto number = fx.components->field(mesh, "f");
        CHECK(number.has_value() && std::get<float>(*number) == 2.5f);
        const auto vec = fx.components->field(mesh, "v");
        CHECK(vec.has_value() && std::get<sky::core::Vec3>(*vec).z == 3.0f);
    }

    std::filesystem::remove_all(std::filesystem::temp_directory_path() /
                                "sky_engine_tests" / "scene");
}

void testLoadRejectsCorruptScene() {
    SceneFixture fx;
    const auto path =
        std::filesystem::temp_directory_path() / "sky_engine_tests" / "scene" / "bad.scene";
    fx.fileSystem->writeAll(path, {std::byte{0xDE}, std::byte{0xAD}});
    CHECK(!fx.scenes->loadScene(path).isValid());
    std::filesystem::remove_all(std::filesystem::temp_directory_path() /
                                "sky_engine_tests" / "scene");
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
    testGuidReferenceResolution();
    testLegacyV11SceneMigrates();
    testLoadRejectsCorruptScene();
    testSceneAuthoring();
    return sky::test::summary("scene_tests");
}
