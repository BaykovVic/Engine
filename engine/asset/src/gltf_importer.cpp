#include <cmath>
#include <cstring>

#include "mini_json.hpp"
#include "sky/asset/gltf_importer.hpp"

namespace sky::asset {
namespace {

using detail::JsonValue;

// --- Small math (column-major mat4, like the backends) -----------------------

struct Mat4 {
    float m[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    Mat4 operator*(const Mat4& other) const {
        Mat4 r;
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    sum += m[k * 4 + row] * other.m[col * 4 + k];
                }
                r.m[col * 4 + row] = sum;
            }
        }
        return r;
    }
};

struct Vec3f {
    float x = 0, y = 0, z = 0;
};

Vec3f transformPoint(const Mat4& m, const Vec3f& v) {
    return {m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z + m.m[12],
            m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z + m.m[13],
            m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z + m.m[14]};
}

Vec3f transformNormal(const Mat4& m, const Vec3f& v) {
    Vec3f n{m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z,
            m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z,
            m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z};
    const float length = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
    if (length > 1e-8f) {
        n.x /= length;
        n.y /= length;
        n.z /= length;
    }
    return n;
}

Mat4 fromTrs(const JsonValue& node) {
    // An explicit matrix wins; otherwise compose T * R * S.
    if (const auto* matrix = node.find("matrix");
        matrix != nullptr && matrix->array.size() == 16) {
        Mat4 r;
        for (int i = 0; i < 16; ++i) {
            r.m[i] = static_cast<float>(matrix->array[i].numberOr(0.0));
        }
        return r;
    }
    float t[3] = {0, 0, 0};
    float q[4] = {0, 0, 0, 1};
    float s[3] = {1, 1, 1};
    if (const auto* translation = node.find("translation")) {
        for (int i = 0; i < 3 && i < int(translation->array.size()); ++i) {
            t[i] = static_cast<float>(translation->array[i].numberOr(0.0));
        }
    }
    if (const auto* rotation = node.find("rotation")) {
        for (int i = 0; i < 4 && i < int(rotation->array.size()); ++i) {
            q[i] = static_cast<float>(rotation->array[i].numberOr(0.0));
        }
    }
    if (const auto* scale = node.find("scale")) {
        for (int i = 0; i < 3 && i < int(scale->array.size()); ++i) {
            s[i] = static_cast<float>(scale->array[i].numberOr(1.0));
        }
    }
    const float xx = q[0] * q[0], yy = q[1] * q[1], zz = q[2] * q[2];
    const float xy = q[0] * q[1], xz = q[0] * q[2], yz = q[1] * q[2];
    const float wx = q[3] * q[0], wy = q[3] * q[1], wz = q[3] * q[2];
    Mat4 r;
    r.m[0] = (1 - 2 * (yy + zz)) * s[0];
    r.m[1] = (2 * (xy + wz)) * s[0];
    r.m[2] = (2 * (xz - wy)) * s[0];
    r.m[4] = (2 * (xy - wz)) * s[1];
    r.m[5] = (1 - 2 * (xx + zz)) * s[1];
    r.m[6] = (2 * (yz + wx)) * s[1];
    r.m[8] = (2 * (xz + wy)) * s[2];
    r.m[9] = (2 * (yz - wx)) * s[2];
    r.m[10] = (1 - 2 * (xx + yy)) * s[2];
    r.m[12] = t[0];
    r.m[13] = t[1];
    r.m[14] = t[2];
    return r;
}

// --- Buffers and accessors ----------------------------------------------------

std::optional<std::vector<std::byte>> decodeBase64(std::string_view text) {
    static const auto decode = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };
    std::vector<std::byte> out;
    out.reserve(text.size() * 3 / 4);
    int accumulator = 0;
    int bits = 0;
    for (const char c : text) {
        if (c == '=' || c == '\n' || c == '\r') {
            continue;
        }
        const int value = decode(c);
        if (value < 0) {
            return std::nullopt;
        }
        accumulator = (accumulator << 6) | value;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(std::byte((accumulator >> bits) & 0xFF));
        }
    }
    return out;
}

struct GltfDocument {
    JsonValue root;
    std::vector<std::vector<std::byte>> buffers;
};

constexpr std::uint32_t kGlbMagic = 0x46546C67; // "glTF"

std::uint32_t readU32At(const std::vector<std::byte>& data, std::size_t offset) {
    std::uint32_t value = 0;
    std::memcpy(&value, data.data() + offset, 4);
    return value;
}

std::optional<GltfDocument> openDocument(platform::IFileSystem& fileSystem,
                                         const std::filesystem::path& path) {
    const auto data = fileSystem.readAll(path);
    if (!data || data->size() < 12) {
        return std::nullopt;
    }

    GltfDocument document;
    std::optional<std::vector<std::byte>> glbBinChunk;

    if (readU32At(*data, 0) == kGlbMagic) {
        // GLB container: header (12) + chunks (length, type, payload).
        if (readU32At(*data, 4) != 2) {
            return std::nullopt; // only glTF 2.0
        }
        std::string jsonText;
        std::size_t offset = 12;
        while (offset + 8 <= data->size()) {
            const auto chunkLength = readU32At(*data, offset);
            const auto chunkType = readU32At(*data, offset + 4);
            if (offset + 8 + chunkLength > data->size()) {
                return std::nullopt;
            }
            const auto* payload = data->data() + offset + 8;
            if (chunkType == 0x4E4F534A) { // "JSON"
                jsonText.assign(reinterpret_cast<const char*>(payload), chunkLength);
            } else if (chunkType == 0x004E4942) { // "BIN"
                glbBinChunk.emplace(payload, payload + chunkLength);
            }
            offset += 8 + chunkLength;
        }
        auto parsed = detail::parseJson(jsonText);
        if (!parsed) {
            return std::nullopt;
        }
        document.root = std::move(*parsed);
    } else {
        auto parsed = detail::parseJson(
            std::string_view(reinterpret_cast<const char*>(data->data()), data->size()));
        if (!parsed) {
            return std::nullopt;
        }
        document.root = std::move(*parsed);
    }

    // Resolve buffers: GLB BIN chunk, data: URIs, or files next to the .gltf.
    if (const auto* buffers = document.root.find("buffers")) {
        for (const auto& buffer : buffers->array) {
            const auto* uri = buffer.find("uri");
            if (uri == nullptr) {
                if (!glbBinChunk) {
                    return std::nullopt;
                }
                document.buffers.push_back(*glbBinChunk);
                continue;
            }
            const auto& text = uri->string;
            constexpr std::string_view kDataPrefix = "data:";
            if (text.starts_with(kDataPrefix)) {
                const auto comma = text.find(',');
                if (comma == std::string::npos) {
                    return std::nullopt;
                }
                auto decoded = decodeBase64(std::string_view(text).substr(comma + 1));
                if (!decoded) {
                    return std::nullopt;
                }
                document.buffers.push_back(std::move(*decoded));
            } else {
                auto external = fileSystem.readAll(path.parent_path() / text);
                if (!external) {
                    return std::nullopt;
                }
                document.buffers.push_back(std::move(*external));
            }
        }
    }
    return document;
}

/// Raw byte view of one accessor element stream, honouring bufferView
/// stride. Returns element pointer for index i.
struct AccessorReader {
    int componentType = 0;
    int componentCount = 0;
    std::size_t count = 0;
    const std::byte* base = nullptr;
    std::size_t stride = 0;

    static std::optional<AccessorReader> open(const GltfDocument& document,
                                              const JsonValue& accessor) {
        AccessorReader reader;
        reader.componentType = int(accessor.find("componentType")
                                       ? accessor.find("componentType")->numberOr(0)
                                       : 0);
        reader.count = std::size_t(
            accessor.find("count") ? accessor.find("count")->numberOr(0) : 0);
        const auto* type = accessor.find("type");
        if (type == nullptr) {
            return std::nullopt;
        }
        if (type->string == "SCALAR") reader.componentCount = 1;
        else if (type->string == "VEC2") reader.componentCount = 2;
        else if (type->string == "VEC3") reader.componentCount = 3;
        else if (type->string == "VEC4") reader.componentCount = 4;
        else return std::nullopt;

        const auto* viewIndex = accessor.find("bufferView");
        const auto* views = document.root.find("bufferViews");
        if (viewIndex == nullptr || views == nullptr) {
            return std::nullopt;
        }
        const auto* view = views->at(std::size_t(viewIndex->numberOr(-1)));
        if (view == nullptr) {
            return std::nullopt;
        }
        const auto bufferIndex = std::size_t(
            view->find("buffer") ? view->find("buffer")->numberOr(0) : 0);
        if (bufferIndex >= document.buffers.size()) {
            return std::nullopt;
        }
        const auto viewOffset = std::size_t(
            view->find("byteOffset") ? view->find("byteOffset")->numberOr(0) : 0);
        const auto accessorOffset = std::size_t(
            accessor.find("byteOffset") ? accessor.find("byteOffset")->numberOr(0)
                                        : 0);
        const std::size_t componentSize =
            reader.componentType == 5126 || reader.componentType == 5125 ? 4
            : reader.componentType == 5123                               ? 2
                                                                         : 1;
        const std::size_t packed = componentSize * reader.componentCount;
        reader.stride = std::size_t(
            view->find("byteStride") ? view->find("byteStride")->numberOr(0) : 0);
        if (reader.stride == 0) {
            reader.stride = packed;
        }
        const auto& buffer = document.buffers[bufferIndex];
        const auto start = viewOffset + accessorOffset;
        if (start + (reader.count > 0 ? (reader.count - 1) * reader.stride + packed
                                      : 0) >
            buffer.size()) {
            return std::nullopt;
        }
        reader.base = buffer.data() + start;
        return reader;
    }

    [[nodiscard]] float floatAt(std::size_t element, int component) const {
        float value = 0.0f;
        std::memcpy(&value, base + element * stride + component * 4, 4);
        return value;
    }

    [[nodiscard]] std::uint32_t indexAt(std::size_t element) const {
        switch (componentType) {
            case 5125: { // u32
                std::uint32_t value;
                std::memcpy(&value, base + element * stride, 4);
                return value;
            }
            case 5123: { // u16
                std::uint16_t value;
                std::memcpy(&value, base + element * stride, 2);
                return value;
            }
            default: { // u8
                return std::uint32_t(*reinterpret_cast<const std::uint8_t*>(
                    base + element * stride));
            }
        }
    }
};

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

bool emitMesh(const GltfDocument& document, std::size_t meshIndex,
              const Mat4& world, std::vector<float>& out) {
    const auto* meshes = document.root.find("meshes");
    const auto* mesh = meshes != nullptr ? meshes->at(meshIndex) : nullptr;
    const auto* accessors = document.root.find("accessors");
    if (mesh == nullptr || accessors == nullptr) {
        return false;
    }
    const auto* primitives = mesh->find("primitives");
    if (primitives == nullptr) {
        return false;
    }

    for (const auto& primitive : primitives->array) {
        // Only triangle lists (the default mode 4).
        if (const auto* mode = primitive.find("mode");
            mode != nullptr && int(mode->numberOr(4)) != 4) {
            continue;
        }
        const auto* attributes = primitive.find("attributes");
        const auto* positionIndex =
            attributes != nullptr ? attributes->find("POSITION") : nullptr;
        if (positionIndex == nullptr) {
            return false;
        }
        const auto* positionAccessor =
            accessors->at(std::size_t(positionIndex->numberOr(-1)));
        if (positionAccessor == nullptr) {
            return false;
        }
        const auto positions = AccessorReader::open(document, *positionAccessor);
        if (!positions || positions->componentType != 5126 ||
            positions->componentCount != 3) {
            return false;
        }

        std::optional<AccessorReader> normals;
        if (const auto* normalIndex = attributes->find("NORMAL")) {
            if (const auto* accessor =
                    accessors->at(std::size_t(normalIndex->numberOr(-1)))) {
                normals = AccessorReader::open(document, *accessor);
            }
        }
        std::optional<AccessorReader> uvs;
        if (const auto* uvIndex = attributes->find("TEXCOORD_0")) {
            if (const auto* accessor =
                    accessors->at(std::size_t(uvIndex->numberOr(-1)))) {
                uvs = AccessorReader::open(document, *accessor);
            }
        }

        // Index stream, or sequential when the primitive is non-indexed.
        std::vector<std::uint32_t> indices;
        if (const auto* indicesIndex = primitive.find("indices")) {
            const auto* accessor =
                accessors->at(std::size_t(indicesIndex->numberOr(-1)));
            if (accessor == nullptr) {
                return false;
            }
            const auto reader = AccessorReader::open(document, *accessor);
            if (!reader) {
                return false;
            }
            indices.resize(reader->count);
            for (std::size_t i = 0; i < reader->count; ++i) {
                indices[i] = reader->indexAt(i);
            }
        } else {
            indices.resize(positions->count);
            for (std::size_t i = 0; i < positions->count; ++i) {
                indices[i] = std::uint32_t(i);
            }
        }
        if (indices.size() % 3 != 0) {
            return false;
        }

        for (std::size_t triangle = 0; triangle < indices.size(); triangle += 3) {
            Vec3f corners[3];
            for (int c = 0; c < 3; ++c) {
                const auto index = indices[triangle + c];
                if (index >= positions->count) {
                    return false;
                }
                corners[c] = transformPoint(
                    world, {positions->floatAt(index, 0), positions->floatAt(index, 1),
                            positions->floatAt(index, 2)});
            }
            const auto computed = flatNormal(corners[0], corners[1], corners[2]);
            for (int c = 0; c < 3; ++c) {
                const auto index = indices[triangle + c];
                Vec3f normal = computed;
                if (normals && index < normals->count) {
                    normal = transformNormal(world,
                                             {normals->floatAt(index, 0),
                                              normals->floatAt(index, 1),
                                              normals->floatAt(index, 2)});
                }
                float u = 0.0f, v = 0.0f;
                if (uvs && index < uvs->count) {
                    u = uvs->floatAt(index, 0);
                    v = uvs->floatAt(index, 1);
                }
                out.insert(out.end(), {corners[c].x, corners[c].y, corners[c].z,
                                       normal.x, normal.y, normal.z, u, v});
            }
        }
    }
    return true;
}

bool emitNode(const GltfDocument& document, std::size_t nodeIndex,
              const Mat4& parentWorld, std::vector<float>& out) {
    const auto* nodes = document.root.find("nodes");
    const auto* node = nodes != nullptr ? nodes->at(nodeIndex) : nullptr;
    if (node == nullptr) {
        return false;
    }
    const Mat4 world = parentWorld * fromTrs(*node);
    if (const auto* mesh = node->find("mesh")) {
        if (!emitMesh(document, std::size_t(mesh->numberOr(-1)), world, out)) {
            return false;
        }
    }
    if (const auto* children = node->find("children")) {
        for (const auto& child : children->array) {
            if (!emitNode(document, std::size_t(child.numberOr(-1)), world, out)) {
                return false;
            }
        }
    }
    return true;
}

} // namespace

std::optional<std::vector<float>> loadGltfMesh(platform::IFileSystem& fileSystem,
                                               const std::filesystem::path& path) {
    const auto document = openDocument(fileSystem, path);
    if (!document) {
        return std::nullopt;
    }

    std::vector<float> mesh;
    const auto* scenes = document->root.find("scenes");
    const auto sceneIndex = std::size_t(
        document->root.find("scene") ? document->root.find("scene")->numberOr(0) : 0);
    const auto* scene = scenes != nullptr ? scenes->at(sceneIndex) : nullptr;
    if (scene != nullptr) {
        if (const auto* roots = scene->find("nodes")) {
            for (const auto& root : roots->array) {
                if (!emitNode(*document, std::size_t(root.numberOr(-1)), Mat4{},
                              mesh)) {
                    return std::nullopt;
                }
            }
        }
    } else if (const auto* meshes = document->root.find("meshes")) {
        // Files without a scene graph: take every mesh at the origin.
        for (std::size_t i = 0; i < meshes->array.size(); ++i) {
            if (!emitMesh(*document, i, Mat4{}, mesh)) {
                return std::nullopt;
            }
        }
    }
    if (mesh.empty()) {
        return std::nullopt;
    }
    return mesh;
}

namespace {

class GltfImporter final : public IAssetImporter {
public:
    explicit GltfImporter(platform::IFileSystem& fileSystem) : fileSystem_(fileSystem) {}

    bool supports(const std::filesystem::path& sourcePath) const override {
        const auto extension = sourcePath.extension();
        return extension == ".gltf" || extension == ".glb";
    }

    std::optional<AssetDescriptor> import(
        const std::filesystem::path& sourcePath) override {
        if (!loadGltfMesh(fileSystem_, sourcePath)) {
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

std::unique_ptr<IAssetImporter> createGltfImporter(platform::IFileSystem& fileSystem) {
    return std::make_unique<GltfImporter>(fileSystem);
}

} // namespace sky::asset
