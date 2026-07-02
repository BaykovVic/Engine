#pragma once

#include <memory>
#include <optional>

#include "sky/package/package_system.hpp"
#include "sky/platform/file_system.hpp"
#include "sky/serialization/serialization.hpp"

namespace sky::package {

/// Name of the package manifest file inside a package directory.
inline constexpr const char* kPackageFileName = "package.skypkg";

/// Local-first implementation of the Package System: discovery from the
/// file system, dependency resolution, activation and extension points.
/// No external registry is ever consulted.
class PackageWorld : public IPackageResolver,
                     public IPackageRegistry,
                     public IExtensionRegistry,
                     public IPackageActivationService {
public:
    ~PackageWorld() override = default;

    /// Scans every subdirectory of `packagesRoot` for a package manifest and
    /// makes the discovered packages available to resolve().
    virtual std::size_t discoverPackages(const std::filesystem::path& packagesRoot) = 0;

    /// Everything discovery has found so far (package management UI).
    [[nodiscard]] virtual std::vector<PackageManifest> discoveredPackages() const = 0;
};

std::unique_ptr<PackageWorld> createPackageWorld(platform::IFileSystem& fileSystem,
                                                 serialization::ISerializationBackend& storage);

/// Writes a manifest to `manifest.rootPath / package.skypkg` so the package
/// becomes discoverable. Used by tooling and tests.
bool savePackageManifest(serialization::ISerializationBackend& storage,
                         const PackageManifest& manifest);

/// Reads the manifest of the package at `packageDir` (nullopt when missing
/// or from a foreign schema). rootPath is set to `packageDir`.
std::optional<PackageManifest> loadPackageManifest(
    serialization::ISerializationBackend& storage,
    const std::filesystem::path& packageDir);

} // namespace sky::package
