#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace sky::serialization {

/// Version of an on-disk schema. Persistence is reproducible: every stored
/// document carries its schema version and can be migrated forward.
struct SchemaVersion {
    std::uint32_t major = 0;
    std::uint32_t minor = 0;

    auto operator<=>(const SchemaVersion&) const = default;
};

/// A serialized document: typed payload plus the schema it was written with.
struct SerializedBlob {
    std::string schemaId;
    SchemaVersion version;
    std::vector<std::byte> payload;
};

/// Serialization and Persistence contract: storage backend for project,
/// scene, asset metadata and terrain data. Owns formats, not domain state.
class ISerializationBackend {
public:
    virtual ~ISerializationBackend() = default;

    virtual bool write(const std::filesystem::path& path, const SerializedBlob& blob) = 0;
    virtual std::optional<SerializedBlob> read(const std::filesystem::path& path) = 0;
};

/// Serialization and Persistence contract: forward migration of stored
/// documents between schema versions.
class ISchemaMigrationService {
public:
    virtual ~ISchemaMigrationService() = default;

    [[nodiscard]] virtual bool canMigrate(const std::string& schemaId,
                                          SchemaVersion from,
                                          SchemaVersion to) const = 0;
    virtual std::optional<SerializedBlob> migrate(const SerializedBlob& blob,
                                                  SchemaVersion target) = 0;
};

} // namespace sky::serialization
