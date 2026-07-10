#include <cmath>
#include <sstream>
#include <string>

#include "sky/asset/obj_importer.hpp"

namespace sky::asset {
namespace {

struct Vec3f {
    float x = 0, y = 0, z = 0;
};

struct Vec2f {
    float u = 0, v = 0;
};

struct FaceVertex {
    int position = 0; // 1-based OBJ index, negative = relative
    int texcoord = 0; // 0 = none
    int normal = 0;   // 0 = none
};

/// Parses "1", "1/2", "1//3" and "1/2/3" face vertex references.
std::optional<FaceVertex> parseFaceVertex(const std::string& token) {
    FaceVertex vertex;
    const auto firstSlash = token.find('/');
    try {
        vertex.position = std::stoi(token.substr(0, firstSlash));
        if (firstSlash != std::string::npos) {
            const auto secondSlash = token.find('/', firstSlash + 1);
            if (secondSlash == std::string::npos) {
                vertex.texcoord = std::stoi(token.substr(firstSlash + 1));
            } else {
                if (secondSlash > firstSlash + 1) {
                    vertex.texcoord = std::stoi(token.substr(
                        firstSlash + 1, secondSlash - firstSlash - 1));
                }
                if (secondSlash + 1 < token.size()) {
                    vertex.normal = std::stoi(token.substr(secondSlash + 1));
                }
            }
        }
    } catch (...) {
        return std::nullopt;
    }
    return vertex;
}

bool resolveIndex(int objIndex, std::size_t count, std::size_t& out) {
    if (objIndex > 0 && static_cast<std::size_t>(objIndex) <= count) {
        out = static_cast<std::size_t>(objIndex - 1);
        return true;
    }
    if (objIndex < 0 && static_cast<std::size_t>(-objIndex) <= count) {
        out = count - static_cast<std::size_t>(-objIndex);
        return true;
    }
    return false;
}

Vec3f flatNormal(const Vec3f& a, const Vec3f& b, const Vec3f& c) {
    const Vec3f u{b.x - a.x, b.y - a.y, b.z - a.z};
    const Vec3f v{c.x - a.x, c.y - a.y, c.z - a.z};
    Vec3f n{u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x};
    const float length = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
    if (length > 1e-8f) {
        n.x /= length;
        n.y /= length;
        n.z /= length;
    } else {
        n = {0.0f, 1.0f, 0.0f};
    }
    return n;
}

} // namespace

std::optional<std::vector<float>> loadObjMesh(platform::IFileSystem& fileSystem,
                                              const std::filesystem::path& path) {
    const auto data = fileSystem.readAll(path);
    if (!data) {
        return std::nullopt;
    }
    std::string text(reinterpret_cast<const char*>(data->data()), data->size());
    std::istringstream stream(text);

    std::vector<Vec3f> positions;
    std::vector<Vec3f> normals;
    std::vector<Vec2f> texcoords;
    std::vector<float> mesh;

    std::string line;
    while (std::getline(stream, line)) {
        std::istringstream record(line);
        std::string keyword;
        record >> keyword;
        if (keyword == "v") {
            Vec3f position;
            if (!(record >> position.x >> position.y >> position.z)) {
                return std::nullopt;
            }
            positions.push_back(position);
        } else if (keyword == "vt") {
            Vec2f uv;
            if (!(record >> uv.u >> uv.v)) {
                return std::nullopt;
            }
            texcoords.push_back(uv);
        } else if (keyword == "vn") {
            Vec3f normal;
            if (!(record >> normal.x >> normal.y >> normal.z)) {
                return std::nullopt;
            }
            normals.push_back(normal);
        } else if (keyword == "f") {
            std::vector<FaceVertex> face;
            std::string token;
            while (record >> token) {
                const auto vertex = parseFaceVertex(token);
                if (!vertex) {
                    return std::nullopt;
                }
                face.push_back(*vertex);
            }
            if (face.size() < 3) {
                return std::nullopt;
            }
            // Fan triangulation: (0, i, i+1) for every polygon.
            for (std::size_t i = 1; i + 1 < face.size(); ++i) {
                const FaceVertex corners[3] = {face[0], face[i], face[i + 1]};
                Vec3f cornerPositions[3];
                for (int c = 0; c < 3; ++c) {
                    std::size_t index = 0;
                    if (!resolveIndex(corners[c].position, positions.size(), index)) {
                        return std::nullopt;
                    }
                    cornerPositions[c] = positions[index];
                }
                const auto computed = flatNormal(cornerPositions[0], cornerPositions[1],
                                                 cornerPositions[2]);
                for (int c = 0; c < 3; ++c) {
                    Vec3f normal = computed;
                    if (corners[c].normal != 0) {
                        std::size_t index = 0;
                        if (!resolveIndex(corners[c].normal, normals.size(), index)) {
                            return std::nullopt;
                        }
                        normal = normals[index];
                    }
                    Vec2f uv;
                    if (corners[c].texcoord != 0) {
                        std::size_t index = 0;
                        if (!resolveIndex(corners[c].texcoord, texcoords.size(),
                                          index)) {
                            return std::nullopt;
                        }
                        uv = texcoords[index];
                    }
                    mesh.insert(mesh.end(),
                                {cornerPositions[c].x, cornerPositions[c].y,
                                 cornerPositions[c].z, normal.x, normal.y, normal.z,
                                 uv.u, uv.v});
                }
            }
        }
        // Other records (vt, o, g, s, usemtl, #, ...) are skipped.
    }
    if (mesh.empty()) {
        return std::nullopt;
    }
    return mesh;
}

namespace {

class ObjImporter final : public IAssetImporter {
public:
    explicit ObjImporter(platform::IFileSystem& fileSystem) : fileSystem_(fileSystem) {}

    bool supports(const std::filesystem::path& sourcePath) const override {
        return sourcePath.extension() == ".obj";
    }

    std::optional<AssetDescriptor> import(
        const std::filesystem::path& sourcePath) override {
        const auto mesh = loadObjMesh(fileSystem_, sourcePath);
        if (!mesh) {
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

std::unique_ptr<IAssetImporter> createObjImporter(platform::IFileSystem& fileSystem) {
    return std::make_unique<ObjImporter>(fileSystem);
}

} // namespace sky::asset
