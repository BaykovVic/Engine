#include <unordered_map>

#include "sky/asset/asset_database.hpp"

namespace sky::asset {
namespace {

class AssetDatabaseImpl final : public AssetDatabase {
public:
    // IAssetResolver

    std::optional<AssetDescriptor> resolve(AssetId id) const override {
        const auto it = assets_.find(id.value);
        if (it == assets_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::optional<AssetId> findBySourcePath(
        const std::filesystem::path& sourcePath) const override {
        const auto id = assetIdFromPath(sourcePath);
        return assets_.contains(id.value) ? std::optional(id) : std::nullopt;
    }

    // IAssetRegistry

    AssetId registerAsset(const AssetDescriptor& descriptor) override {
        assets_[descriptor.id.value] = descriptor;
        return descriptor.id;
    }

    void unregisterAsset(AssetId id) override { assets_.erase(id.value); }

    std::vector<AssetId> dependentsOf(AssetId id) const override {
        std::vector<AssetId> result;
        for (const auto& [assetId, descriptor] : assets_) {
            for (const auto dependency : descriptor.dependencies) {
                if (dependency == id) {
                    result.push_back(AssetId{assetId});
                    break;
                }
            }
        }
        return result;
    }

    std::vector<AssetDescriptor> allAssets() const override {
        std::vector<AssetDescriptor> result;
        result.reserve(assets_.size());
        for (const auto& [id, descriptor] : assets_) {
            result.push_back(descriptor);
        }
        return result;
    }

    // IImportPipeline

    void registerImporter(IAssetImporter& importer) override {
        importers_.push_back(&importer);
    }

    std::optional<AssetId> importAsset(const std::filesystem::path& sourcePath) override {
        for (auto* importer : importers_) {
            if (!importer->supports(sourcePath)) {
                continue;
            }
            auto descriptor = importer->import(sourcePath);
            if (!descriptor) {
                return std::nullopt;
            }
            // The pipeline owns identity: importers fill in type, metadata
            // and dependencies; the id always comes from the source path.
            descriptor->id = assetIdFromPath(sourcePath);
            descriptor->sourcePath = sourcePath;
            ++descriptor->contentVersion;
            return registerAsset(*descriptor);
        }
        return std::nullopt;
    }

    bool reimport(AssetId id) override {
        const auto it = assets_.find(id.value);
        if (it == assets_.end()) {
            return false;
        }
        const auto previousVersion = it->second.contentVersion;
        const auto sourcePath = it->second.sourcePath;
        for (auto* importer : importers_) {
            if (!importer->supports(sourcePath)) {
                continue;
            }
            auto descriptor = importer->import(sourcePath);
            if (!descriptor) {
                return false;
            }
            descriptor->id = id;
            descriptor->sourcePath = sourcePath;
            descriptor->contentVersion = previousVersion + 1;
            registerAsset(*descriptor);
            return true;
        }
        return false;
    }

private:
    std::unordered_map<std::uint64_t, AssetDescriptor> assets_;
    std::vector<IAssetImporter*> importers_;
};

} // namespace

AssetId assetIdFromPath(const std::filesystem::path& sourcePath) {
    // FNV-1a over the generic (separator-normalized) path string.
    const auto text = sourcePath.lexically_normal().generic_string();
    std::uint64_t hash = 1469598103934665603ull;
    for (const char c : text) {
        hash ^= static_cast<unsigned char>(c);
        hash *= 1099511628211ull;
    }
    // Reserve 0 as the invalid id.
    return AssetId{hash == 0 ? 1 : hash};
}

std::unique_ptr<AssetDatabase> createAssetDatabase() {
    return std::make_unique<AssetDatabaseImpl>();
}

} // namespace sky::asset
