// OBJ import and the material library: the asset-facing half of the visual
// pipeline (lighting itself is verified through the GL backend rendering).

#include <cmath>
#include <filesystem>
#include <string>

#include "sky/asset/asset_database.hpp"
#include "sky/asset/obj_importer.hpp"
#include "sky/platform/platform_services.hpp"
#include "sky/rendering/material.hpp"
#include "sky/rendering/null_renderer.hpp"
#include "sky_test.hpp"

namespace {

std::filesystem::path testRoot() {
    return std::filesystem::temp_directory_path() / "sky_engine_tests";
}

void writeText(sky::platform::IFileSystem& fileSystem,
               const std::filesystem::path& path, const std::string& text) {
    std::vector<std::byte> bytes(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        bytes[i] = static_cast<std::byte>(text[i]);
    }
    fileSystem.writeAll(path, bytes);
}

void testObjParsing() {
    const auto fileSystem = sky::platform::createStdFileSystem();

    // A quad with explicit normals, using v//vn references: two triangles.
    writeText(*fileSystem, testRoot() / "quad.obj",
              "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\n"
              "vn 0 0 1\n"
              "f 1//1 2//1 3//1\nf 1//1 3//1 4//1\n");
    const auto quad = sky::asset::loadObjMesh(*fileSystem, testRoot() / "quad.obj");
    CHECK(quad.has_value());
    CHECK(quad->size() == 2u * 3u * 6u);
    // The provided normal is used verbatim.
    CHECK((*quad)[3] == 0.0f);
    CHECK((*quad)[5] == 1.0f);

    // A pyramid without normals and with a quad base: fan triangulation
    // plus computed flat normals.
    writeText(*fileSystem, testRoot() / "pyramid.obj",
              "v -1 0 -1\nv 1 0 -1\nv 1 0 1\nv -1 0 1\nv 0 1.8 0\n"
              "f 1 2 5\nf 2 3 5\nf 3 4 5\nf 4 1 5\nf 4 3 2 1\n");
    const auto pyramid =
        sky::asset::loadObjMesh(*fileSystem, testRoot() / "pyramid.obj");
    CHECK(pyramid.has_value());
    // 4 side triangles + 2 from the quad base.
    CHECK(pyramid->size() == 6u * 3u * 6u);
    // Computed normals are unit length.
    const float nx = (*pyramid)[3], ny = (*pyramid)[4], nz = (*pyramid)[5];
    CHECK(std::fabs(std::sqrt(nx * nx + ny * ny + nz * nz) - 1.0f) < 1e-3f);

    // Negative (relative) indices resolve from the end of the list.
    writeText(*fileSystem, testRoot() / "relative.obj",
              "v 0 0 0\nv 1 0 0\nv 0 1 0\nf -3 -2 -1\n");
    CHECK(sky::asset::loadObjMesh(*fileSystem, testRoot() / "relative.obj")
              .has_value());

    // Garbage and out-of-range indices are rejected, not misparsed.
    writeText(*fileSystem, testRoot() / "broken.obj", "v 0 0\nf 1 2 9\n");
    CHECK(!sky::asset::loadObjMesh(*fileSystem, testRoot() / "broken.obj")
               .has_value());
    CHECK(!sky::asset::loadObjMesh(*fileSystem, testRoot() / "missing.obj")
               .has_value());

    // The importer plugs into the asset pipeline.
    const auto database = sky::asset::createAssetDatabase();
    const auto importer = sky::asset::createObjImporter(*fileSystem);
    database->registerImporter(*importer);
    const auto id = database->importAsset(testRoot() / "pyramid.obj");
    CHECK(id.has_value());
    CHECK(database->resolve(*id)->assetType == "mesh");
    CHECK(!database->importAsset(testRoot() / "broken.obj").has_value());

    // The parsed mesh uploads through the rendering contract.
    const auto renderer = sky::rendering::createNullRenderer();
    CHECK(renderer->createMeshFromData(*pyramid).isValid());

    fileSystem->remove(testRoot());
}

void testMaterialLibrary() {
    const auto library = sky::rendering::createMaterialLibrary();

    const auto gold = library->createMaterial(
        {"Gold", {1.0f, 0.8f, 0.3f}, 0.2f, 1.0f, {}});
    CHECK(gold.isValid());
    // Names are unique; empty names are rejected.
    CHECK(!library->createMaterial({"Gold", {}, 0.5f, 0.0f, {}}).isValid());
    CHECK(!library->createMaterial({"", {}, 0.5f, 0.0f, {}}).isValid());

    CHECK(library->findMaterial("Gold") == gold);
    CHECK(!library->findMaterial("Chrome").has_value());
    CHECK(library->material(gold).metallic == 1.0f);

    // Updates change parameters and support renames with index upkeep.
    CHECK(library->updateMaterial(gold, {"Rose Gold", {1.0f, 0.6f, 0.5f}, 0.3f,
                                         1.0f, {}}));
    CHECK(!library->findMaterial("Gold").has_value());
    CHECK(library->findMaterial("Rose Gold") == gold);
    CHECK(library->material(gold).baseColor.y == 0.6f);

    const auto glow = library->createMaterial(
        {"Glow", {0.2f, 0.5f, 0.9f}, 0.9f, 0.0f, {0.1f, 0.3f, 0.6f}});
    CHECK(glow.isValid());
    // Renaming onto an existing name is refused.
    CHECK(!library->updateMaterial(glow, {"Rose Gold", {}, 0.5f, 0.0f, {}}));
    CHECK(library->allMaterials().size() == 2);
    CHECK(library->material(glow).emissive.z == 0.6f);
}

} // namespace

int main() {
    testObjParsing();
    testMaterialLibrary();
    return sky::test::summary("visual_pipeline_tests");
}
