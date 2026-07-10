#include <cctype>
#include <cmath>

#include "ofbx.h"
#include "sky/asset/fbx_importer.hpp"

namespace sky::asset {
namespace {

struct Vec3f {
    float x = 0, y = 0, z = 0;
};

Vec3f transformPoint(const ofbx::DMatrix& m, const ofbx::Vec3& v) {
    return {static_cast<float>(m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z + m.m[12]),
            static_cast<float>(m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z + m.m[13]),
            static_cast<float>(m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z + m.m[14])};
}

Vec3f transformNormal(const ofbx::DMatrix& m, const ofbx::Vec3& v) {
    Vec3f n{static_cast<float>(m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z),
            static_cast<float>(m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z),
            static_cast<float>(m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z)};
    const float length = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
    if (length > 1e-8f) {
        n.x /= length;
        n.y /= length;
        n.z /= length;
    }
    return n;
}

} // namespace

std::optional<std::vector<float>> loadFbxMesh(platform::IFileSystem& fileSystem,
                                              const std::filesystem::path& path) {
    const auto data = fileSystem.readAll(path);
    if (!data) {
        return std::nullopt;
    }
    ofbx::IScene* scene =
        ofbx::load(reinterpret_cast<const ofbx::u8*>(data->data()), data->size(),
                   static_cast<ofbx::u16>(ofbx::LoadFlags::NONE));
    if (scene == nullptr) {
        return std::nullopt;
    }

    std::vector<float> mesh;
    std::vector<int> triIndices;
    for (int meshIndex = 0; meshIndex < scene->getMeshCount(); ++meshIndex) {
        const auto* fbxMesh = scene->getMesh(meshIndex);
        const auto& geometry = fbxMesh->getGeometryData();
        const auto positions = geometry.getPositions();
        const auto normals = geometry.getNormals();
        const auto uvs = geometry.getUVs();
        if (positions.count == 0) {
            continue;
        }
        const auto worldTransform = fbxMesh->getGlobalTransform();

        for (int partitionIndex = 0; partitionIndex < geometry.getPartitionCount();
             ++partitionIndex) {
            const auto partition = geometry.getPartition(partitionIndex);
            triIndices.resize(
                static_cast<std::size_t>(partition.max_polygon_triangles) * 3);
            for (int polygonIndex = 0; polygonIndex < partition.polygon_count;
                 ++polygonIndex) {
                const auto& polygon = partition.polygons[polygonIndex];
                const auto triangleVertices =
                    ofbx::triangulate(geometry, polygon, triIndices.data());
                for (ofbx::u32 i = 0; i < triangleVertices; ++i) {
                    const int index = triIndices[i];
                    const auto position =
                        transformPoint(worldTransform, positions.get(index));
                    Vec3f normal{0.0f, 1.0f, 0.0f};
                    if (normals.count > index) {
                        normal = transformNormal(worldTransform, normals.get(index));
                    }
                    float u = 0.0f, v = 0.0f;
                    if (uvs.count > index) {
                        const auto uv = uvs.get(index);
                        u = static_cast<float>(uv.x);
                        v = static_cast<float>(uv.y);
                    }
                    mesh.insert(mesh.end(), {position.x, position.y, position.z,
                                             normal.x, normal.y, normal.z, u, v});
                }
            }
        }
    }
    scene->destroy();
    if (mesh.empty()) {
        return std::nullopt;
    }
    return mesh;
}

namespace {

class FbxImporter final : public IAssetImporter {
public:
    explicit FbxImporter(platform::IFileSystem& fileSystem) : fileSystem_(fileSystem) {}

    bool supports(const std::filesystem::path& sourcePath) const override {
        auto extension = sourcePath.extension().string();
        for (auto& c : extension) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return extension == ".fbx";
    }

    std::optional<AssetDescriptor> import(
        const std::filesystem::path& sourcePath) override {
        if (!loadFbxMesh(fileSystem_, sourcePath)) {
            return std::nullopt;
        }
        AssetDescriptor descriptor;
        descriptor.assetType = "mesh";
        return descriptor;
    }

private:
    platform::IFileSystem& fileSystem_;
};

} // namespace

std::unique_ptr<IAssetImporter> createFbxImporter(platform::IFileSystem& fileSystem) {
    return std::make_unique<FbxImporter>(fileSystem);
}

} // namespace sky::asset
