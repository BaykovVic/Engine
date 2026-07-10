#include <map>

#include "sky/serialization/backends.hpp"

namespace sky::serialization {
namespace {

struct MigrationKey {
    std::string schemaId;
    SchemaVersion from;

    auto operator<=>(const MigrationKey&) const = default;
};

class SchemaMigrationServiceImpl final : public SchemaMigrationService {
public:
    void registerMigration(const std::string& schemaId, SchemaVersion from,
                           SchemaVersion to, Migration migration) override {
        migrations_[{schemaId, from}] = {to, std::move(migration)};
    }

    bool canMigrate(const std::string& schemaId, SchemaVersion from,
                    SchemaVersion to) const override {
        // Follow the registered chain stepwise until the target is reached.
        SchemaVersion cursor = from;
        for (std::size_t guard = 0; guard < migrations_.size() + 1; ++guard) {
            if (cursor == to) {
                return true;
            }
            const auto it = migrations_.find({schemaId, cursor});
            if (it == migrations_.end()) {
                return false;
            }
            cursor = it->second.first;
        }
        return false;
    }

    std::optional<SerializedBlob> migrate(const SerializedBlob& blob,
                                          SchemaVersion target) override {
        if (!canMigrate(blob.schemaId, blob.version, target)) {
            return std::nullopt;
        }
        SerializedBlob current = blob;
        while (current.version != target) {
            const auto it = migrations_.find({current.schemaId, current.version});
            auto payload = it->second.second(current.payload);
            if (!payload) {
                return std::nullopt;
            }
            current.payload = std::move(*payload);
            current.version = it->second.first;
        }
        return current;
    }

private:
    std::map<MigrationKey, std::pair<SchemaVersion, Migration>> migrations_;
};

} // namespace

std::unique_ptr<SchemaMigrationService> createSchemaMigrationService() {
    return std::make_unique<SchemaMigrationServiceImpl>();
}

} // namespace sky::serialization
