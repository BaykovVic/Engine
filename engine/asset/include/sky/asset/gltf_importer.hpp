#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "sky/asset/asset_system.hpp"
#include "sky/platform/file_system.hpp"

namespace sky::asset {

/// Loads every mesh of a glTF 2.0 file (.gltf with external/base64 buffers,
/// or the binary .glb container) into interleaved position(3) + normal(3) +
/// uv(2) triangles. Node transforms are baked in; flat normals are computed
/// when the file has none. Own implementation — JSON parsing included.
std::optional<std::vector<float>> loadGltfMesh(platform::IFileSystem& fileSystem,
                                               const std::filesystem::path& path);

/// Asset System importer for .gltf/.glb sources, registered as type "mesh".
std::unique_ptr<IAssetImporter> createGltfImporter(platform::IFileSystem& fileSystem);

} // namespace sky::asset
