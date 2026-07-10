#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace sky::asset {

/// Stable asset identity. Survives renames and moves; everything that
/// references an asset references its AssetId, never a path.
struct AssetId {
    std::uint64_t value = 0;

    [[nodiscard]] bool isValid() const noexcept { return value != 0; }
    auto operator<=>(const AssetId&) const = default;
};

struct AssetDescriptor {
    AssetId id;
    std::string assetType;
    std::filesystem::path sourcePath;
    std::vector<AssetId> dependencies;
    std::uint64_t contentVersion = 0;
};

/// Asset System contract: resolve an AssetId to its descriptor and data.
class IAssetResolver {
public:
    virtual ~IAssetResolver() = default;

    [[nodiscard]] virtual std::optional<AssetDescriptor> resolve(AssetId id) const = 0;
    [[nodiscard]] virtual std::optional<AssetId> findBySourcePath(
        const std::filesystem::path& sourcePath) const = 0;
};

/// Asset System contract: registry owning asset identity, metadata and the
/// dependency graph.
class IAssetRegistry {
public:
    virtual ~IAssetRegistry() = default;

    virtual AssetId registerAsset(const AssetDescriptor& descriptor) = 0;
    virtual void unregisterAsset(AssetId id) = 0;
    [[nodiscard]] virtual std::vector<AssetId> dependentsOf(AssetId id) const = 0;
    [[nodiscard]] virtual std::vector<AssetDescriptor> allAssets() const = 0;
};

/// A single importer for one family of source formats. Contributed by core
/// or by packages via extension points.
class IAssetImporter {
public:
    virtual ~IAssetImporter() = default;

    [[nodiscard]] virtual bool supports(const std::filesystem::path& sourcePath) const = 0;
    virtual std::optional<AssetDescriptor> import(const std::filesystem::path& sourcePath) = 0;
};

/// Asset System contract: import orchestration. Picks the importer, assigns
/// identity, updates metadata and the dependency graph.
class IImportPipeline {
public:
    virtual ~IImportPipeline() = default;

    virtual void registerImporter(IAssetImporter& importer) = 0;
    virtual std::optional<AssetId> importAsset(const std::filesystem::path& sourcePath) = 0;
    virtual bool reimport(AssetId id) = 0;
};

} // namespace sky::asset
