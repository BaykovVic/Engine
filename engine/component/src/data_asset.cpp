#include <variant>

#include "sky/component/data_asset.hpp"
#include "sky/serialization/byte_stream.hpp"

namespace sky::component {
namespace {

constexpr const char* kDataSchemaId = "sky.data";
// 1.0: typeId + parent GUID + the field map.
constexpr serialization::SchemaVersion kDataSchemaVersion{1, 0};

// Field encoding shared with the scene schema by convention (see
// scene_world.cpp); duplicated here so the two formats stay independently
// versioned.
enum class FieldTag : std::uint32_t {
    Float = 0,
    Int = 1,
    Bool = 2,
    String = 3,
    Vec3 = 4,
};

void writeFieldValue(serialization::ByteWriter& writer, const FieldValue& value) {
    if (const auto* f = std::get_if<float>(&value)) {
        writer.writeU32(static_cast<std::uint32_t>(FieldTag::Float));
        writer.writeF32(*f);
    } else if (const auto* i = std::get_if<std::int64_t>(&value)) {
        writer.writeU32(static_cast<std::uint32_t>(FieldTag::Int));
        writer.writeU64(static_cast<std::uint64_t>(*i));
    } else if (const auto* b = std::get_if<bool>(&value)) {
        writer.writeU32(static_cast<std::uint32_t>(FieldTag::Bool));
        writer.writeU32(*b ? 1 : 0);
    } else if (const auto* s = std::get_if<std::string>(&value)) {
        writer.writeU32(static_cast<std::uint32_t>(FieldTag::String));
        writer.writeString(*s);
    } else if (const auto* v = std::get_if<core::Vec3>(&value)) {
        writer.writeU32(static_cast<std::uint32_t>(FieldTag::Vec3));
        writer.writeF32(v->x);
        writer.writeF32(v->y);
        writer.writeF32(v->z);
    }
}

std::optional<FieldValue> readFieldValue(serialization::ByteReader& reader) {
    const auto tag = reader.readU32();
    if (!tag) {
        return std::nullopt;
    }
    switch (static_cast<FieldTag>(*tag)) {
        case FieldTag::Float:
            if (const auto value = reader.readF32()) {
                return FieldValue{*value};
            }
            break;
        case FieldTag::Int:
            if (const auto value = reader.readU64()) {
                return FieldValue{static_cast<std::int64_t>(*value)};
            }
            break;
        case FieldTag::Bool:
            if (const auto value = reader.readU32()) {
                return FieldValue{*value != 0};
            }
            break;
        case FieldTag::String:
            if (auto value = reader.readString()) {
                return FieldValue{std::move(*value)};
            }
            break;
        case FieldTag::Vec3: {
            const auto x = reader.readF32(), y = reader.readF32(),
                       z = reader.readF32();
            if (x && y && z) {
                return FieldValue{core::Vec3{*x, *y, *z}};
            }
            break;
        }
    }
    return std::nullopt;
}

} // namespace

bool saveDataAsset(serialization::ISerializationBackend& storage,
                   const std::filesystem::path& path, const DataAssetDesc& desc) {
    serialization::ByteWriter writer;
    writer.writeString(desc.typeId);
    writer.writeU64(desc.parentGuid);
    writer.writeU32(static_cast<std::uint32_t>(desc.fields.size()));
    for (const auto& [name, value] : desc.fields) {
        writer.writeString(name);
        writeFieldValue(writer, value);
    }
    return storage.write(path,
                         {kDataSchemaId, kDataSchemaVersion, writer.takeBuffer()});
}

std::optional<DataAssetDesc> loadDataAsset(
    serialization::ISerializationBackend& storage,
    const std::filesystem::path& path) {
    const auto blob = storage.read(path);
    if (!blob || blob->schemaId != kDataSchemaId ||
        blob->version != kDataSchemaVersion) {
        return std::nullopt;
    }
    serialization::ByteReader reader(blob->payload);
    DataAssetDesc desc;
    const auto typeId = reader.readString();
    const auto parentGuid = reader.readU64();
    const auto fieldCount = reader.readU32();
    if (!typeId || !parentGuid || !fieldCount) {
        return std::nullopt;
    }
    desc.typeId = *typeId;
    desc.parentGuid = *parentGuid;
    for (std::uint32_t i = 0; i < *fieldCount; ++i) {
        const auto name = reader.readString();
        auto value = readFieldValue(reader);
        if (!name || !value) {
            return std::nullopt;
        }
        desc.fields.emplace(*name, std::move(*value));
    }
    return desc;
}

std::map<std::string, FieldValue> mergedFields(
    const std::map<std::string, FieldValue>& base,
    const std::map<std::string, FieldValue>& overrides) {
    auto result = base;
    for (const auto& [name, value] : overrides) {
        result[name] = value;
    }
    return result;
}

} // namespace sky::component
