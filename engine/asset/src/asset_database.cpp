#include <charconv>
#include <random>
#include <string>
#include <unordered_map>

#include "sky/asset/asset_database.hpp"
#include "sky/platform/file_system.hpp"

namespace sky::asset {
namespace {

/// Sidecar file next to the source: `crate.png` -> `crate.png.skymeta`.
std::filesystem::path sidecarPath(const std::filesystem::path& sourcePath) {
    return std::filesystem::path(sourcePath.string() + ".skymeta");
}

class AssetDatabaseImpl final : public AssetDatabase {
public:
    explicit AssetDatabaseImpl(platform::IFileSystem* fileSystem)
        : fileSystem_(fileSystem) {}

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
        // A real lookup over the registered descriptors — valid for both
        // identity policies (GUIDs cannot be recomputed from the path).
        const auto needle = sourcePath.lexically_normal().generic_string();
        for (const auto& [id, descriptor] : assets_) {
            if (descriptor.sourcePath.lexically_normal().generic_string() ==
                needle) {
                return AssetId{id};
            }
        }
        return std::nullopt;
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
            // and dependencies; the id comes from the sidecar GUID (stable
            // across renames) or, without a file system, the source path.
            descriptor->id = identityFor(sourcePath);
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
    AssetId identityFor(const std::filesystem::path& sourcePath) {
        if (fileSystem_ == nullptr) {
            return assetIdFromPath(sourcePath);
        }
        const auto sidecar = sidecarPath(sourcePath);
        if (const auto bytes = fileSystem_->readAll(sidecar)) {
            if (const auto guid = parseSidecar(*bytes)) {
                return AssetId{*guid};
            }
        }
        const auto guid = freshGuid();
        const std::string text = "guid=" + toHex(guid) + "\n";
        std::vector<std::byte> bytes(text.size());
        for (std::size_t i = 0; i < text.size(); ++i) {
            bytes[i] = static_cast<std::byte>(text[i]);
        }
        fileSystem_->writeAll(sidecar, bytes);
        return AssetId{guid};
    }

    static std::optional<std::uint64_t> parseSidecar(
        const std::vector<std::byte>& bytes) {
        std::string text(bytes.size(), '\0');
        for (std::size_t i = 0; i < bytes.size(); ++i) {
            text[i] = static_cast<char>(bytes[i]);
        }
        const auto key = text.find("guid=");
        if (key == std::string::npos) {
            return std::nullopt;
        }
        const char* first = text.data() + key + 5;
        const char* last = text.data() + text.size();
        std::uint64_t value = 0;
        const auto [ptr, ec] = std::from_chars(first, last, value, 16);
        if (ec != std::errc{} || value == 0) {
            return std::nullopt;
        }
        return value;
    }

    static std::string toHex(std::uint64_t value) {
        char buffer[17] = {};
        const auto [ptr, ec] = std::to_chars(buffer, buffer + 16, value, 16);
        return std::string(buffer, ptr);
    }

    std::uint64_t freshGuid() {
        std::uint64_t guid = 0;
        // Nonzero (0 = invalid) and unique within this database.
        while (guid == 0 || assets_.contains(guid)) {
            guid = rng_();
        }
        return guid;
    }

    platform::IFileSystem* fileSystem_ = nullptr;
    std::mt19937_64 rng_{std::random_device{}()};
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
    return std::make_unique<AssetDatabaseImpl>(nullptr);
}

std::unique_ptr<AssetDatabase> createAssetDatabase(
    platform::IFileSystem& fileSystem) {
    return std::make_unique<AssetDatabaseImpl>(&fileSystem);
}

} // namespace sky::asset
