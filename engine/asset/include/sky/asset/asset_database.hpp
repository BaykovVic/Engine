#pragma once

#include <memory>

#include "sky/asset/asset_system.hpp"

namespace sky::platform {
class IFileSystem;
}

namespace sky::asset {

/// In-memory implementation of the Asset System: identity, registry,
/// dependency graph and import orchestration in one database.
///
/// Identity policy depends on the factory used. With a file system the id
/// is a GUID persisted in a `<file>.skymeta` sidecar next to the source —
/// the sidecar moves with the file, so renames keep the identity. Without
/// one the id falls back to a hash of the normalized source path.
class AssetDatabase : public IAssetResolver, public IAssetRegistry, public IImportPipeline {
public:
    ~AssetDatabase() override = default;
};

/// Path-hash identity only (no sidecars). Renaming a source file yields a
/// new id; suited to tests and generated in-memory pipelines.
std::unique_ptr<AssetDatabase> createAssetDatabase();

/// Sidecar GUID identity: the first import of a file writes
/// `<file>.skymeta` carrying a fresh GUID; later imports (including after
/// a rename, as long as the sidecar travelled with the file) reuse it.
std::unique_ptr<AssetDatabase> createAssetDatabase(platform::IFileSystem& fileSystem);

/// Stable asset identity derived from the normalized source path (FNV-1a).
/// The fallback identity when no sidecar file system is attached.
AssetId assetIdFromPath(const std::filesystem::path& sourcePath);

} // namespace sky::asset
