#include "sky/serialization/backends.hpp"
#include "sky/serialization/byte_stream.hpp"

namespace sky::serialization {
namespace {

// Container format magic: "SKYB" little-endian.
constexpr std::uint32_t kMagic = 0x42594B53;

class FileSerializationBackend final : public ISerializationBackend {
public:
    explicit FileSerializationBackend(platform::IFileSystem& fileSystem)
        : fileSystem_(fileSystem) {}

    bool write(const std::filesystem::path& path, const SerializedBlob& blob) override {
        ByteWriter writer;
        writer.writeU32(kMagic);
        writer.writeString(blob.schemaId);
        writer.writeU32(blob.version.major);
        writer.writeU32(blob.version.minor);
        writer.writeBytes(blob.payload);
        return fileSystem_.writeAll(path, writer.buffer());
    }

    std::optional<SerializedBlob> read(const std::filesystem::path& path) override {
        const auto data = fileSystem_.readAll(path);
        if (!data) {
            return std::nullopt;
        }
        ByteReader reader(*data);
        if (reader.readU32() != kMagic) {
            return std::nullopt;
        }
        SerializedBlob blob;
        const auto schemaId = reader.readString();
        const auto major = reader.readU32();
        const auto minor = reader.readU32();
        auto payload = reader.readBytes();
        if (!schemaId || !major || !minor || !payload) {
            return std::nullopt;
        }
        blob.schemaId = *schemaId;
        blob.version = {*major, *minor};
        blob.payload = std::move(*payload);
        return blob;
    }

private:
    platform::IFileSystem& fileSystem_;
};

} // namespace

std::unique_ptr<ISerializationBackend> createFileSerializationBackend(
    platform::IFileSystem& fileSystem) {
    return std::make_unique<FileSerializationBackend>(fileSystem);
}

} // namespace sky::serialization
