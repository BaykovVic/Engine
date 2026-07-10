#include <algorithm>
#include <cstdlib>

#include "sky/package/package_installer.hpp"
#include "sky/package/package_world.hpp"
#include "sky/package/semver.hpp"

namespace sky::package {
namespace {

bool runCommand(const std::string& command) {
    return std::system((command + " > /dev/null 2>&1").c_str()) == 0;
}

std::string shellQuote(const std::filesystem::path& path) {
    return "\"" + path.string() + "\"";
}

/// Replaces `to` with a recursive copy of `from`; a cloned repository's
/// .git directory is dropped (installed packages are plain content).
bool copyTree(const std::filesystem::path& from, const std::filesystem::path& to) {
    std::error_code ec;
    std::filesystem::remove_all(to, ec);
    std::filesystem::create_directories(to.parent_path(), ec);
    std::filesystem::copy(from, to,
                          std::filesystem::copy_options::recursive |
                              std::filesystem::copy_options::copy_symlinks,
                          ec);
    if (ec) {
        return false;
    }
    std::filesystem::remove_all(to / ".git", ec);
    return true;
}

/// The package directory inside an extracted/cloned tree: the root itself
/// when it carries a manifest, otherwise the first subdirectory that does.
std::optional<std::filesystem::path> findPackageDir(
    const std::filesystem::path& root) {
    std::error_code ec;
    if (std::filesystem::exists(root / kPackageFileName, ec)) {
        return root;
    }
    for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
        if (entry.is_directory() &&
            std::filesystem::exists(entry.path() / kPackageFileName, ec)) {
            return entry.path();
        }
    }
    return std::nullopt;
}

bool isTarball(const std::string& source) {
    std::string lower = source;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return lower.ends_with(".tar.gz") || lower.ends_with(".tgz") ||
           lower.ends_with(".tar");
}

} // namespace

PackageInstaller::PackageInstaller(serialization::ISerializationBackend& storage,
                                   std::filesystem::path cacheRoot)
    : storage_(storage), cacheRoot_(std::move(cacheRoot)) {}

std::optional<PackageManifest> PackageInstaller::install(
    const std::string& source, const std::filesystem::path& projectPackages) {
    std::error_code ec;
    std::optional<PackageManifest> cached;

    if (isTarball(source) && std::filesystem::is_regular_file(source, ec)) {
        const auto staging = cacheRoot_ / ".staging";
        std::filesystem::remove_all(staging, ec);
        std::filesystem::create_directories(staging, ec);
        if (!runCommand("tar -xf " + shellQuote(source) + " -C " + shellQuote(staging))) {
            return std::nullopt;
        }
        if (const auto dir = findPackageDir(staging)) {
            cached = cachePackageDir(*dir);
        }
        std::filesystem::remove_all(staging, ec);
    } else if (std::filesystem::is_directory(source, ec) &&
               std::filesystem::exists(std::filesystem::path(source) /
                                       kPackageFileName,
                                       ec)) {
        cached = cachePackageDir(source);
    } else {
        // Anything else is treated as git; "url#tag" pins a tag or branch.
        std::string url = source;
        std::string tag;
        if (const auto hash = source.rfind('#'); hash != std::string::npos) {
            url = source.substr(0, hash);
            tag = source.substr(hash + 1);
        }
        const auto staging = cacheRoot_ / ".staging";
        std::filesystem::remove_all(staging, ec);
        std::filesystem::create_directories(staging, ec);
        const auto clone = staging / "repo";
        std::string command = "git clone --quiet --depth 1 ";
        if (!tag.empty()) {
            command += "--branch \"" + tag + "\" ";
        }
        command += shellQuote(url) + " " + shellQuote(clone);
        if (!runCommand(command)) {
            std::filesystem::remove_all(staging, ec);
            return std::nullopt;
        }
        if (const auto dir = findPackageDir(clone)) {
            cached = cachePackageDir(*dir);
        }
        std::filesystem::remove_all(staging, ec);
    }

    if (!cached) {
        return std::nullopt;
    }
    // Project copy lives in a version-qualified directory, so several
    // versions can sit side by side for the resolver.
    const auto target = projectPackages /
                        (cached->packageId + "-" + cached->version);
    if (!copyTree(cached->rootPath, target)) {
        return std::nullopt;
    }
    cached->rootPath = target;
    return cached;
}

std::optional<PackageManifest> PackageInstaller::cachePackageDir(
    const std::filesystem::path& packageDir) {
    auto manifest = loadPackageManifest(storage_, packageDir);
    if (!manifest || !parseVersion(manifest->version)) {
        return std::nullopt;
    }
    const auto cacheDir = cacheRoot_ / manifest->packageId / manifest->version;
    std::error_code ec;
    // The cache is immutable: an already-cached version is reused as-is.
    if (!std::filesystem::exists(cacheDir / kPackageFileName, ec) &&
        !copyTree(packageDir, cacheDir)) {
        return std::nullopt;
    }
    manifest->rootPath = cacheDir;
    return manifest;
}

} // namespace sky::package
