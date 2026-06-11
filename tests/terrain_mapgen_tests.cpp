#include <cmath>
#include <filesystem>

#include "sky/mapgen/generation_pipeline.hpp"
#include "sky/platform/platform_services.hpp"
#include "sky/serialization/backends.hpp"
#include "sky/terrain/terrain_world.hpp"
#include "sky_test.hpp"

namespace {

sky::terrain::TerrainDataset flatDataset(std::uint32_t resolution) {
    sky::terrain::TerrainDataset dataset;
    dataset.resolution = resolution;
    dataset.chunkSize = resolution;
    dataset.worldScale = {1.0f, 1.0f, 1.0f};
    dataset.heights.assign(static_cast<std::size_t>(resolution) * resolution, 0.0f);
    return dataset;
}

void testTerrainEditingAndHooks() {
    const auto fileSystem = sky::platform::createStdFileSystem();
    const auto storage = sky::serialization::createFileSerializationBackend(*fileSystem);
    const auto terrain = sky::terrain::createTerrainWorld(*storage);

    int changeNotifications = 0;
    terrain->onTerrainChanged([&](sky::terrain::TerrainHandle) { ++changeNotifications; });

    // Inconsistent datasets are rejected.
    sky::terrain::TerrainDataset broken;
    broken.resolution = 4;
    CHECK(!terrain->createTerrain(broken).isValid());

    const auto handle = terrain->createTerrain(flatDataset(16));
    CHECK(handle.isValid());
    CHECK(changeNotifications == 1);

    // Raise a hill in the middle: the centre rises the most, the edge of
    // the brush stays untouched.
    terrain->applyEdit(handle, {{8.0f, 0.0f, 8.0f}, 4.0f, 2.0f, "raise"});
    CHECK(changeNotifications == 2);
    const float centre = terrain->heightAt(handle, 8.0f, 8.0f);
    CHECK(std::fabs(centre - 2.0f) < 1e-3f);
    CHECK(terrain->heightAt(handle, 8.0f, 6.0f) < centre);
    CHECK(terrain->heightAt(handle, 0.0f, 0.0f) == 0.0f);

    // Bilinear interpolation between grid points.
    const float between = terrain->heightAt(handle, 8.0f, 7.5f);
    CHECK(between > terrain->heightAt(handle, 8.0f, 7.0f));
    CHECK(between < centre);

    // Lower undoes raise; smooth flattens the hill.
    terrain->applyEdit(handle, {{8.0f, 0.0f, 8.0f}, 4.0f, 2.0f, "lower"});
    CHECK(std::fabs(terrain->heightAt(handle, 8.0f, 8.0f)) < 1e-3f);
}

void testTerrainPersistence() {
    const auto fileSystem = sky::platform::createStdFileSystem();
    const auto storage = sky::serialization::createFileSerializationBackend(*fileSystem);
    const auto path = (std::filesystem::temp_directory_path() / "sky_engine_tests" /
                       "ground.skyterrain").string();

    {
        const auto terrain = sky::terrain::createTerrainWorld(*storage);
        const auto handle = terrain->createTerrain(flatDataset(8));
        terrain->applyEdit(handle, {{4.0f, 0.0f, 4.0f}, 3.0f, 1.5f, "raise"});
        terrain->setStoragePath(handle, path);
        CHECK(terrain->save(handle));
    }

    {
        const auto terrain = sky::terrain::createTerrainWorld(*storage);
        const auto handle = terrain->load(path);
        CHECK(handle.isValid());
        CHECK(terrain->dataset(handle).resolution == 8);
        CHECK(std::fabs(terrain->heightAt(handle, 4.0f, 4.0f) - 1.5f) < 1e-3f);
        // The loaded terrain can be saved back to the same place.
        CHECK(terrain->save(handle));
    }

    std::filesystem::remove_all(std::filesystem::temp_directory_path() /
                                "sky_engine_tests");
}

void testGenerationDeterminism() {
    const auto pipeline = sky::mapgen::createGenerationPipeline();
    CHECK(pipeline->availableStages().size() == 2);

    sky::mapgen::GenerationProfile profile;
    profile.profileId = "default";
    profile.seed = 12345;
    profile.mapSize = 32;
    profile.enabledStages = {sky::mapgen::kStageHeightfield,
                             sky::mapgen::kStagePlacement};

    const auto first = pipeline->generate({profile, {}});
    const auto second = pipeline->generate({profile, {}});
    CHECK(first->succeeded());

    // Same profile and seed: bit-identical terrain and placements.
    CHECK(first->terrainOutput().heights == second->terrainOutput().heights);
    CHECK(first->placements().size() == second->placements().size());
    CHECK(first->placements().front().transform.position ==
          second->placements().front().transform.position);

    // A different seed produces a different world.
    profile.seed = 54321;
    const auto other = pipeline->generate({profile, {}});
    CHECK(other->terrainOutput().heights != first->terrainOutput().heights);

    // Disabled stages stay inert.
    profile.enabledStages = {sky::mapgen::kStageHeightfield};
    CHECK(pipeline->generate({profile, {}})->placements().empty());

    // Generated output flows into the terrain world (the Map Generation ->
    // Terrain and Landscape edge of the architecture).
    const auto fileSystem = sky::platform::createStdFileSystem();
    const auto storage = sky::serialization::createFileSerializationBackend(*fileSystem);
    const auto terrain = sky::terrain::createTerrainWorld(*storage);
    const auto handle = terrain->createTerrain(flatDataset(32));
    terrain->replaceDataset(handle, first->terrainOutput());
    CHECK(terrain->dataset(handle).heights == first->terrainOutput().heights);
}

} // namespace

int main() {
    testTerrainEditingAndHooks();
    testTerrainPersistence();
    testGenerationDeterminism();
    return sky::test::summary("terrain_mapgen_tests");
}
