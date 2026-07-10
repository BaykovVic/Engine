#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "sky/package/package_system.hpp"
#include "sky/serialization/serialization.hpp"

namespace sky::package {

/// Installs packages into a project from three kinds of sources:
///   - a local package directory (contains package.skypkg),
///   - a tarball (.tar / .tar.gz / .tgz),
///   - a git repository URL or path ("url#tag" pins a tag or branch).
/// Every install lands in an immutable, version-addressed global cache
/// (<cacheRoot>/<id>/<version>) first and is copied into the project's
/// Packages directory from there, so repeated installs of the same version
/// never re-fetch. Extraction and cloning shell out to tar/git.
class PackageInstaller {
public:
    PackageInstaller(serialization::ISerializationBackend& storage,
                     std::filesystem::path cacheRoot);

    /// Returns the installed manifest with rootPath pointing at the copy
    /// under `projectPackages`, or nullopt on any failure (bad source,
    /// missing manifest, tool failure).
    std::optional<PackageManifest> install(
        const std::string& source, const std::filesystem::path& projectPackages);

    [[nodiscard]] const std::filesystem::path& cacheRoot() const {
        return cacheRoot_;
    }

private:
    std::optional<PackageManifest> cachePackageDir(
        const std::filesystem::path& packageDir);

    serialization::ISerializationBackend& storage_;
    std::filesystem::path cacheRoot_;
};

} // namespace sky::package
