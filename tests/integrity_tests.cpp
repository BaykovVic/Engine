// Covers the DoD items that close the Scene/Object/Component/ECS integration
// risks called out in the architecture docs: the explicit ECS sync contract,
// component field persistence with schema migration, terrain integration
// with physics and rendering, and generation materialized into the scene.

#include <cmath>
#include <filesystem>

#include "sky/component/component_world.hpp"
#include "sky/ecs/ecs_world.hpp"
#include "sky/ecs/object_sync.hpp"
#include "sky/mapgen/generation_pipeline.hpp"
#include "sky/mapgen/materialize.hpp"
#include "sky/object/object_world.hpp"
#include "sky/physics/physics_world.hpp"
#include "sky/platform/platform_services.hpp"
#include "sky/rendering/null_renderer.hpp"
#include "sky/scene/scene_world.hpp"
#include "sky/serialization/backends.hpp"
#include "sky/serialization/byte_stream.hpp"
#include "sky/terrain/terrain_integration.hpp"
#include "sky/terrain/terrain_world.hpp"
#include "sky_test.hpp"

namespace {

std::filesystem::path testRoot() {
    return std::filesystem::temp_directory_path() / "sky_engine_tests" / "integrity";
}

/// Doubles the X position of every bound entity — a data-oriented system
/// working purely on ECS state.
class DoubleXSystem final : public sky::ecs::IEcsSystem {
public:
    explicit DoubleXSystem(sky::ecs::EcsWorld& world) : world_(world) {}
    std::string name() const override { return "double-x"; }
    void update(double) override {
        for (const auto entity : world_.entitiesWith({typeid(sky::ecs::EcsTransform)})) {
            world_.storeFor<sky::ecs::EcsTransform>().get(entity)->value.position.x *=
                2.0f;
        }
    }

private:
    sky::ecs::EcsWorld& world_;
};

void testEcsObjectSync() {
    const auto objects = sky::object::createObjectWorld();
    const auto ecs = sky::ecs::createEcsWorld();
    const auto sync = sky::ecs::createEcsObjectSync(*ecs, *objects);

    const auto hero = objects->createObject("hero");
    objects->setLocalTransform(hero, {{3.0f, 0.0f, 0.0f}, {}, {1, 1, 1}});
    const auto entity = sync->bind(hero);
    CHECK(entity.isValid());
    CHECK(sync->entityOf(hero) == entity);
    CHECK(sync->objectOf(entity) == hero);
    // Binding twice reuses the entity.
    CHECK(sync->bind(hero) == entity);

    // Authoring -> ECS: the entity sees the object's transform.
    objects->setLocalTransform(hero, {{5.0f, 0.0f, 0.0f}, {}, {1, 1, 1}});
    sync->pushAuthoringState();
    CHECK(ecs->storeFor<sky::ecs::EcsTransform>().get(entity)->value.position.x == 5.0f);

    // ECS -> authoring: system results flow back to the object.
    DoubleXSystem system(*ecs);
    ecs->registerSystem(system);
    ecs->tick(1.0 / 60.0);
    sync->pullEcsResults();
    CHECK(objects->localTransform(hero).position.x == 10.0f);
    ecs->unregisterSystem(system);

    sync->unbind(hero);
    CHECK(!sync->entityOf(hero).isValid());
    CHECK(!ecs->isAlive(entity));
}

struct SceneFixture {
    std::unique_ptr<sky::object::ObjectWorld> objects =
        sky::object::createObjectWorld();
    std::unique_ptr<sky::component::ComponentWorld> components =
        sky::component::createComponentWorld();
    std::unique_ptr<sky::platform::IFileSystem> fileSystem =
        sky::platform::createStdFileSystem();
    std::unique_ptr<sky::serialization::ISerializationBackend> storage =
        sky::serialization::createFileSerializationBackend(*fileSystem);
    std::unique_ptr<sky::serialization::SchemaMigrationService> migrations =
        sky::serialization::createSchemaMigrationService();
    std::unique_ptr<sky::scene::SceneWorld> scenes;

    SceneFixture() {
        sky::scene::SceneWorldDeps deps{*objects,    *objects,   *objects,
                                        *components, *components, *storage};
        deps.componentData = components.get();
        deps.migrations = migrations.get();
        scenes = sky::scene::createSceneWorld(deps);
        components->registerComponentType(
            {"sky.script", "Script", true, "Game.Mover",
             {{"speed", "float"}, {"path", "string"}}});
    }
};

void testComponentFieldRoundTrip() {
    const auto path = testRoot() / "fields.scene";

    {
        SceneFixture fx;
        const auto scene = fx.scenes->createScene({"fields", path});
        const auto hero = fx.objects->createObject("hero");
        fx.scenes->addRootObject(scene, hero);
        const auto script = fx.components->attach(hero, "sky.script");

        fx.components->setField(script, "speed", 4.5f);
        fx.components->setField(script, "lives", std::int64_t{3});
        fx.components->setField(script, "invincible", true);
        fx.components->setField(script, "path", std::string("patrol-route"));
        fx.components->setField(script, "spawn", sky::core::Vec3{1.0f, 2.0f, 3.0f});
        CHECK(fx.components->fields(script).size() == 5);

        CHECK(fx.scenes->saveScene(scene));
    }

    {
        SceneFixture fx;
        const auto scene = fx.scenes->loadScene(path);
        CHECK(scene.isValid());
        const auto hero = fx.objects->findByName("hero").front();
        const auto script = fx.components->componentsOf(hero).front();

        CHECK(std::get<float>(*fx.components->field(script, "speed")) == 4.5f);
        CHECK(std::get<std::int64_t>(*fx.components->field(script, "lives")) == 3);
        CHECK(std::get<bool>(*fx.components->field(script, "invincible")) == true);
        CHECK(std::get<std::string>(*fx.components->field(script, "path")) ==
              "patrol-route");
        CHECK(std::get<sky::core::Vec3>(*fx.components->field(script, "spawn")) ==
              (sky::core::Vec3{1.0f, 2.0f, 3.0f}));
        CHECK(!fx.components->field(script, "missing").has_value());
    }

    std::filesystem::remove_all(testRoot());
}

void testLegacySceneMigration() {
    const auto path = testRoot() / "legacy.scene";
    SceneFixture fx;

    // Hand-craft a v1.0 scene file: one object with one component and no
    // field section, exactly as the old writer produced it.
    sky::serialization::ByteWriter writer;
    writer.writeString("legacy");
    writer.writeU32(1);
    writer.writeU32(0xFFFFFFFFu); // root
    writer.writeString("relic");
    for (const float value : {0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f}) {
        writer.writeF32(value);
    }
    writer.writeU32(1);
    writer.writeString("sky.script");
    CHECK(fx.storage->write(path, {"sky.scene", {1, 0}, writer.takeBuffer()}));

    CHECK(fx.migrations->canMigrate("sky.scene", {1, 0}, {1, 1}));
    const auto scene = fx.scenes->loadScene(path);
    CHECK(scene.isValid());

    const auto relics = fx.objects->findByName("relic");
    CHECK(relics.size() == 1);
    CHECK(fx.objects->localTransform(relics.front()).position.y == 7.0f);
    CHECK(fx.components->componentsOf(relics.front()).size() == 1);

    std::filesystem::remove_all(testRoot());
}

sky::terrain::TerrainDataset hillDataset() {
    sky::terrain::TerrainDataset dataset;
    dataset.resolution = 16;
    dataset.chunkSize = 16;
    dataset.worldScale = {1.0f, 1.0f, 1.0f};
    dataset.heights.assign(16 * 16, 0.0f);
    // A flat-topped hill around (8, 8).
    for (std::uint32_t z = 6; z <= 10; ++z) {
        for (std::uint32_t x = 6; x <= 10; ++x) {
            dataset.heights[z * 16 + x] = 4.0f;
        }
    }
    return dataset;
}

void testTerrainPhysicsIntegration() {
    const auto physics = sky::physics::createPhysicsWorld();
    const auto dataset = hillDataset();

    const auto terrainBody = physics->createBody({sky::physics::BodyType::Static, 0.0f, {}});
    physics->attachCollider(terrainBody, sky::terrain::makeTerrainCollider(dataset));

    // A crate dropped over the hill must land on the hilltop (y = 4 + 0.5)…
    const auto onHill = physics->createBody(
        {sky::physics::BodyType::Dynamic, 1.0f, {{8.0f, 10.0f, 8.0f}, {}, {1, 1, 1}}});
    physics->attachCollider(onHill, {sky::physics::ColliderShape::Box,
                                     {0.5f, 0.5f, 0.5f}, 0.0f});
    // …and one dropped at the flats must land at ground level (y = 0.5).
    const auto onFlat = physics->createBody(
        {sky::physics::BodyType::Dynamic, 1.0f, {{2.0f, 10.0f, 2.0f}, {}, {1, 1, 1}}});
    physics->attachCollider(onFlat, {sky::physics::ColliderShape::Box,
                                     {0.5f, 0.5f, 0.5f}, 0.0f});

    bool collided = false;
    for (int i = 0; i < 240; ++i) {
        physics->step(1.0 / 60.0);
        if (!physics->drainCollisionEvents().empty()) {
            collided = true;
        }
    }
    CHECK(collided);
    CHECK(std::fabs(physics->bodyTransform(onHill).position.y - 4.5f) < 0.05f);
    CHECK(std::fabs(physics->bodyTransform(onFlat).position.y - 0.5f) < 0.05f);
}

void testTerrainRenderIntegration() {
    const auto dataset = hillDataset();
    const auto mesh = sky::terrain::buildTerrainMesh(dataset);

    // (16-1)^2 cells * 2 triangles * 3 vertices * 8 floats (pos+normal+uv).
    CHECK(mesh.size() == 15u * 15u * 2u * 3u * 8u);

    // Normals are unit-length; flat areas point straight up.
    const float nx = mesh[3], ny = mesh[4], nz = mesh[5];
    CHECK(std::fabs(std::sqrt(nx * nx + ny * ny + nz * nz) - 1.0f) < 1e-3f);
    CHECK(ny > 0.99f);

    // The mesh uploads through the rendering contract.
    const auto renderer = sky::rendering::createNullRenderer();
    const auto resource = renderer->createMeshFromData(mesh);
    CHECK(resource.isValid());
    CHECK(!renderer->createMeshFromData(std::vector<float>(7, 0.0f)).isValid());

    // Degenerate datasets produce no mesh.
    CHECK(sky::terrain::buildTerrainMesh({}).empty());
}

void testGenerationMaterialization() {
    SceneFixture fx;
    const auto terrain = sky::terrain::createTerrainWorld(*fx.storage);
    const auto pipeline = sky::mapgen::createGenerationPipeline();

    sky::terrain::TerrainDataset flat;
    flat.resolution = 32;
    flat.chunkSize = 8;
    flat.worldScale = {1.0f, 1.0f, 1.0f};
    flat.heights.assign(32 * 32, 0.0f);
    const auto terrainHandle = terrain->createTerrain(flat);

    sky::mapgen::GenerationProfile profile;
    profile.profileId = "default";
    profile.seed = 99;
    profile.mapSize = 32;
    profile.enabledStages = {sky::mapgen::kStageHeightfield,
                             sky::mapgen::kStagePlacement};
    const auto result = pipeline->generate({profile, terrainHandle});
    CHECK(result->succeeded());

    const auto scene = fx.scenes->createScene({"generated", {}});
    const auto created = sky::mapgen::materializeGenerationResult(
        *result, *terrain, terrainHandle, *fx.scenes, scene, *fx.objects, *fx.objects);

    // Terrain dataset replaced and placements materialized as scene roots.
    CHECK(terrain->dataset(terrainHandle).heights == result->terrainOutput().heights);
    CHECK(created.size() == result->placements().size());
    CHECK(!created.empty());
    fx.scenes->activate(scene);
    CHECK(fx.scenes->activeContext().rootObjects.size() == created.size());
    CHECK(fx.scenes->sceneOf(created.front()) == scene);
    CHECK(fx.objects->localTransform(created.front()).position ==
          result->placements().front().transform.position);
    // Failed results materialize nothing.
    sky::mapgen::GenerationProfile bad;
    bad.mapSize = 0;
    const auto failed = pipeline->generate({bad, terrainHandle});
    CHECK(sky::mapgen::materializeGenerationResult(*failed, *terrain, terrainHandle,
                                                   *fx.scenes, scene, *fx.objects,
                                                   *fx.objects)
              .empty());
}

} // namespace

int main() {
    testEcsObjectSync();
    testComponentFieldRoundTrip();
    testLegacySceneMigration();
    testTerrainPhysicsIntegration();
    testTerrainRenderIntegration();
    testGenerationMaterialization();
    return sky::test::summary("integrity_tests");
}
