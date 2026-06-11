#pragma once

#include <memory>

#include "sky/asset/asset_system.hpp"

namespace sky::asset {

/// In-memory implementation of the Asset System: identity, registry,
/// dependency graph and import orchestration in one database.
///
/// AssetIds are stable: they are derived from the normalized source path,
/// so re-importing or reopening a project yields the same identity.
class AssetDatabase : public IAssetResolver, public IAssetRegistry, public IImportPipeline {
public:
    ~AssetDatabase() override = default;
};

std::unique_ptr<AssetDatabase> createAssetDatabase();

/// Stable asset identity derived from the normalized source path (FNV-1a).
AssetId assetIdFromPath(const std::filesystem::path& sourcePath);

} // namespace sky::asset
