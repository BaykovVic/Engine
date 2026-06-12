// OBJ import and the material library: the asset-facing half of the visual
// pipeline (lighting itself is verified through the GL backend rendering).

#include <cmath>
#include <cstring>
#include <filesystem>
#include <string>

#include "sky/asset/asset_database.hpp"
#include "sky/asset/fbx_importer.hpp"
#include "sky/asset/gltf_importer.hpp"
#include "sky/asset/obj_importer.hpp"
#include "sky/asset/png_decoder.hpp"
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
    CHECK(quad->size() == 2u * 3u * 8u);
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
    CHECK(pyramid->size() == 6u * 3u * 8u);
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

void testUvParsing() {
    const auto fileSystem = sky::platform::createStdFileSystem();
    // v/vt/vn references: UVs land in floats 6..7 of each vertex.
    writeText(*fileSystem, testRoot() / "uv.obj",
              "v 0 0 0\nv 1 0 0\nv 1 1 0\n"
              "vt 0 0\nvt 1 0\nvt 1 1\n"
              "vn 0 0 1\n"
              "f 1/1/1 2/2/1 3/3/1\n");
    const auto mesh = sky::asset::loadObjMesh(*fileSystem, testRoot() / "uv.obj");
    CHECK(mesh.has_value());
    CHECK(mesh->size() == 3u * 8u);
    CHECK((*mesh)[6] == 0.0f && (*mesh)[7] == 0.0f);
    CHECK((*mesh)[8 + 6] == 1.0f && (*mesh)[8 + 7] == 0.0f);
    CHECK((*mesh)[16 + 6] == 1.0f && (*mesh)[16 + 7] == 1.0f);
    fileSystem->remove(testRoot());
}

void testFbxImport() {
    const auto fileSystem = sky::platform::createStdFileSystem();
    const std::filesystem::path sample = SKY_TEST_DATA_DIR "/box.fbx";

    const auto mesh = sky::asset::loadFbxMesh(*fileSystem, sample);
    CHECK(mesh.has_value());
    // The engine vertex format: whole triangles of 8-float vertices.
    CHECK(!mesh->empty());
    CHECK(mesh->size() % 24 == 0);
    // A box triangulates to 12 triangles.
    CHECK(mesh->size() == 12u * 3u * 8u);

    // The importer registers FBX as a mesh asset; garbage is rejected.
    const auto database = sky::asset::createAssetDatabase();
    const auto importer = sky::asset::createFbxImporter(*fileSystem);
    database->registerImporter(*importer);
    const auto id = database->importAsset(sample);
    CHECK(id.has_value());
    CHECK(database->resolve(*id)->assetType == "mesh");
    writeText(*fileSystem, testRoot() / "fake.fbx", "not an fbx");
    CHECK(!database->importAsset(testRoot() / "fake.fbx").has_value());

    // Uploads through the rendering contract.
    const auto renderer = sky::rendering::createNullRenderer();
    CHECK(renderer->createMeshFromData(*mesh).isValid());
    fileSystem->remove(testRoot());
}

namespace gltf_data {

// One triangle: positions (3 x vec3 float) then u16 indices [0,1,2] + pad.
// Layout: 36 bytes positions + 8 bytes indices = 44 bytes total.
const unsigned char kTriangleBin[] = {
    0, 0, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0,   // (0,0,0)
    0, 0, 128, 63, 0, 0, 0, 0,   0, 0, 0, 0,   // (1,0,0)
    0, 0, 0, 0,   0, 0, 128, 63, 0, 0, 0, 0,   // (0,1,0)
    0, 0, 1, 0, 2, 0, 0, 0,                    // indices u16 + pad
};

std::string base64Encode(const unsigned char* data, std::size_t size) {
    static const char* alphabet =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    for (std::size_t i = 0; i < size; i += 3) {
        const unsigned a = data[i];
        const unsigned b = i + 1 < size ? data[i + 1] : 0;
        const unsigned c = i + 2 < size ? data[i + 2] : 0;
        out += alphabet[a >> 2];
        out += alphabet[((a & 3) << 4) | (b >> 4)];
        out += i + 1 < size ? alphabet[((b & 15) << 2) | (c >> 6)] : '=';
        out += i + 2 < size ? alphabet[c & 63] : '=';
    }
    return out;
}

/// A .gltf with an embedded base64 buffer: one indexed triangle inside a
/// node translated by (5, 0, 0).
std::string triangleGltfJson() {
    return std::string(R"({
  "asset": {"version": "2.0"},
  "scene": 0,
  "scenes": [{"nodes": [0]}],
  "nodes": [{"mesh": 0, "translation": [5, 0, 0]}],
  "meshes": [{"primitives": [{
      "attributes": {"POSITION": 0}, "indices": 1}]}],
  "accessors": [
    {"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3"},
    {"bufferView": 1, "componentType": 5123, "count": 3, "type": "SCALAR"}],
  "bufferViews": [
    {"buffer": 0, "byteOffset": 0, "byteLength": 36},
    {"buffer": 0, "byteOffset": 36, "byteLength": 6}],
  "buffers": [{"byteLength": 44,
    "uri": "data:application/octet-stream;base64,)" +
           base64Encode(kTriangleBin, sizeof(kTriangleBin)) + R"("}]
})");
}

/// The same document as a binary .glb container.
std::vector<std::byte> triangleGlb() {
    std::string json = R"({
  "asset": {"version": "2.0"},
  "scenes": [{"nodes": [0]}],
  "nodes": [{"mesh": 0}],
  "meshes": [{"primitives": [{"attributes": {"POSITION": 0}, "indices": 1}]}],
  "accessors": [
    {"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3"},
    {"bufferView": 1, "componentType": 5123, "count": 3, "type": "SCALAR"}],
  "bufferViews": [
    {"buffer": 0, "byteOffset": 0, "byteLength": 36},
    {"buffer": 0, "byteOffset": 36, "byteLength": 6}],
  "buffers": [{"byteLength": 44}]
})";
    while (json.size() % 4 != 0) {
        json += ' ';
    }
    std::vector<std::byte> bin(sizeof(kTriangleBin));
    std::memcpy(bin.data(), kTriangleBin, sizeof(kTriangleBin));
    while (bin.size() % 4 != 0) {
        bin.push_back(std::byte{0});
    }

    std::vector<std::byte> out;
    const auto push32 = [&](std::uint32_t value) {
        for (int i = 0; i < 4; ++i) {
            out.push_back(std::byte((value >> (i * 8)) & 0xFF));
        }
    };
    push32(0x46546C67); // magic "glTF"
    push32(2);
    push32(std::uint32_t(12 + 8 + json.size() + 8 + bin.size()));
    push32(std::uint32_t(json.size()));
    push32(0x4E4F534A); // "JSON"
    for (const char c : json) {
        out.push_back(std::byte(c));
    }
    push32(std::uint32_t(bin.size()));
    push32(0x004E4942); // "BIN"
    out.insert(out.end(), bin.begin(), bin.end());
    return out;
}

} // namespace gltf_data

void testGltfImport() {
    const auto fileSystem = sky::platform::createStdFileSystem();

    // Text .gltf with an embedded base64 buffer and a node translation.
    writeText(*fileSystem, testRoot() / "tri.gltf", gltf_data::triangleGltfJson());
    const auto mesh = sky::asset::loadGltfMesh(*fileSystem, testRoot() / "tri.gltf");
    CHECK(mesh.has_value());
    CHECK(mesh->size() == 3u * 8u);
    // The node transform is baked: vertex 0 = (0,0,0) + (5,0,0).
    CHECK((*mesh)[0] == 5.0f && (*mesh)[1] == 0.0f);
    CHECK((*mesh)[8] == 6.0f);  // (1,0,0) translated
    CHECK((*mesh)[17] == 1.0f); // (0,1,0) translated, y stays 1
    // Computed flat normal points +Z.
    CHECK(std::fabs((*mesh)[5] - 1.0f) < 1e-4f);

    // The binary .glb container parses to the same triangle (no transform).
    fileSystem->writeAll(testRoot() / "tri.glb", gltf_data::triangleGlb());
    const auto glb = sky::asset::loadGltfMesh(*fileSystem, testRoot() / "tri.glb");
    CHECK(glb.has_value());
    CHECK(glb->size() == 3u * 8u);
    CHECK((*glb)[8] == 1.0f); // untranslated (1,0,0)

    // The importer pipeline accepts both spellings and rejects garbage.
    const auto database = sky::asset::createAssetDatabase();
    const auto importer = sky::asset::createGltfImporter(*fileSystem);
    database->registerImporter(*importer);
    CHECK(database->importAsset(testRoot() / "tri.gltf").has_value());
    CHECK(database->importAsset(testRoot() / "tri.glb").has_value());
    writeText(*fileSystem, testRoot() / "bad.gltf", "{not json");
    CHECK(!database->importAsset(testRoot() / "bad.gltf").has_value());
    writeText(*fileSystem, testRoot() / "bad.glb", "glTFgarbage");
    CHECK(!sky::asset::loadGltfMesh(*fileSystem, testRoot() / "bad.glb").has_value());

    // Uploads through the rendering contract.
    const auto renderer = sky::rendering::createNullRenderer();
    CHECK(renderer->createMeshFromData(*mesh).isValid());
    fileSystem->remove(testRoot());
}

void testPngRoundTrip() {
    const auto fileSystem = sky::platform::createStdFileSystem();

    // Encode a small gradient image and decode it back, pixel-exact.
    sky::asset::ImageData image;
    image.width = 5;
    image.height = 3;
    image.pixels.resize(5 * 3 * 4);
    for (std::size_t i = 0; i < image.pixels.size(); ++i) {
        image.pixels[i] = static_cast<std::uint8_t>(i * 7 % 256);
    }
    const auto encoded = sky::asset::encodePngRgba(image);
    const auto decoded = sky::asset::decodePng(encoded);
    CHECK(decoded.has_value());
    CHECK(decoded->width == 5 && decoded->height == 3);
    CHECK(decoded->pixels == image.pixels);

    // The importer accepts the file and registers it as a texture.
    fileSystem->writeAll(testRoot() / "img.png", encoded);
    const auto database = sky::asset::createAssetDatabase();
    const auto importer = sky::asset::createPngImporter(*fileSystem);
    database->registerImporter(*importer);
    const auto id = database->importAsset(testRoot() / "img.png");
    CHECK(id.has_value());
    CHECK(database->resolve(*id)->assetType == "texture");

    // Corrupt signatures and truncated streams are rejected.
    CHECK(!sky::asset::decodePng({std::byte{1}, std::byte{2}}).has_value());
    auto truncated = encoded;
    truncated.resize(encoded.size() / 2);
    CHECK(!sky::asset::decodePng(truncated).has_value());

    // Textures upload through the rendering contract with validation.
    const auto renderer = sky::rendering::createNullRenderer();
    CHECK(renderer->createTextureFromData(decoded->width, decoded->height,
                                          decoded->pixels)
              .isValid());
    CHECK(!renderer->createTextureFromData(4, 4, decoded->pixels).isValid());
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
    testUvParsing();
    testFbxImport();
    testGltfImport();
    testPngRoundTrip();
    testMaterialLibrary();
    return sky::test::summary("visual_pipeline_tests");
}
