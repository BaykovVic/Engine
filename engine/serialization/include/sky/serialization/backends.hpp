#pragma once

#include <functional>
#include <memory>
#include <string>

#include "sky/platform/file_system.hpp"
#include "sky/serialization/serialization.hpp"

namespace sky::serialization {

/// Serialization backend storing blobs in Sky Engine's container format:
/// magic + schema id + schema version + payload, via the given file system.
std::unique_ptr<ISerializationBackend> createFileSerializationBackend(
    platform::IFileSystem& fileSystem);

/// Concrete migration service: modules register stepwise payload
/// transformations and the service chains them to reach the target version.
class SchemaMigrationService : public ISchemaMigrationService {
public:
    ~SchemaMigrationService() override = default;

    using Migration = std::function<std::optional<std::vector<std::byte>>(
        const std::vector<std::byte>& payload)>;

    virtual void registerMigration(const std::string& schemaId, SchemaVersion from,
                                   SchemaVersion to, Migration migration) = 0;
};

std::unique_ptr<SchemaMigrationService> createSchemaMigrationService();

} // namespace sky::serialization
