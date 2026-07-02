#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "sky/package/package_system.hpp"
#include "sky/serialization/serialization.hpp"

namespace sky::package {

/// Conventional lock file name inside a project's Packages directory.
inline constexpr const char* kPackageLockFileName = "sky.lock";

/// One resolved package pinned by the lock: the exact version that
/// resolution chose, a checksum of its manifest (drift detection), and
/// whether the project has it active.
struct LockedPackage {
    std::string packageId;
    std::string version;
    std::uint64_t manifestChecksum = 0;
    bool active = false;
};

/// FNV-1a over the manifest's identity-defining fields. A changed
/// dependency list or version bumps the checksum; the rootPath does not
/// participate (moving a package is not drift).
std::uint64_t manifestChecksum(const PackageManifest& manifest);

bool savePackageLock(serialization::ISerializationBackend& storage,
                     const std::filesystem::path& path,
                     const std::vector<LockedPackage>& packages);

/// Nullopt when the file is missing or from a different schema.
std::optional<std::vector<LockedPackage>> loadPackageLock(
    serialization::ISerializationBackend& storage,
    const std::filesystem::path& path);

} // namespace sky::package
