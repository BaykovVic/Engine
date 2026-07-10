#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "sky/asset/asset_system.hpp"
#include "sky/platform/file_system.hpp"

namespace sky::asset {

/// Parses a Wavefront OBJ file into interleaved position(3) + normal(3)
/// triangles, ready for IRenderResourceFactory::createMeshFromData.
/// Supports v/vn/f records (f as v, v//vn or v/vt/vn, with fan
/// triangulation); flat normals are computed when the file has none.
/// Own implementation — no third-party importer dependencies.
std::optional<std::vector<float>> loadObjMesh(platform::IFileSystem& fileSystem,
                                              const std::filesystem::path& path);

/// Asset System importer for .obj sources: validates the mesh and registers
/// it with type "mesh".
std::unique_ptr<IAssetImporter> createObjImporter(platform::IFileSystem& fileSystem);

} // namespace sky::asset
