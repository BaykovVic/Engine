#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "sky/asset/asset_system.hpp"
#include "sky/platform/file_system.hpp"

namespace sky::asset {

/// Loads every mesh of an FBX file (binary or ASCII, via the vendored
/// OpenFBX, MIT) into interleaved position(3) + normal(3) + uv(2)
/// triangles in the engine's mesh format. Node transforms are baked in.
std::optional<std::vector<float>> loadFbxMesh(platform::IFileSystem& fileSystem,
                                              const std::filesystem::path& path);

/// Asset System importer for .fbx sources, registered as type "mesh".
std::unique_ptr<IAssetImporter> createFbxImporter(platform::IFileSystem& fileSystem);

} // namespace sky::asset
