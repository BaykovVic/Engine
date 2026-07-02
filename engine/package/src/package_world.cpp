#include <map>
#include <unordered_map>
#include <unordered_set>

#include "sky/package/package_lock.hpp"
#include "sky/package/package_world.hpp"
#include "sky/package/semver.hpp"
#include "sky/serialization/byte_stream.hpp"

namespace sky::package {
namespace {

constexpr const char* kPackageSchemaId = "sky.package";
constexpr serialization::SchemaVersion kPackageSchemaVersion{1, 0};

std::optional<PackageManifest> readManifest(serialization::ISerializationBackend& storage,
                                            const std::filesystem::path& packageDir) {
    const auto blob = storage.read(packageDir / kPackageFileName);
    if (!blob || blob->schemaId != kPackageSchemaId ||
        blob->version != kPackageSchemaVersion) {
        return std::nullopt;
    }

    serialization::ByteReader reader(blob->payload);
    PackageManifest manifest;
    manifest.rootPath = packageDir;

    const auto packageId = reader.readString();
    const auto version = reader.readString();
    const auto displayName = reader.readString();
    const auto dependencyCount = reader.readU32();
    if (!packageId || !version || !displayName || !dependencyCount) {
        return std::nullopt;
    }
    manifest.packageId = *packageId;
    manifest.version = *version;
    manifest.displayName = *displayName;

    for (std::uint32_t i = 0; i < *dependencyCount; ++i) {
        const auto id = reader.readString();
        const auto requirement = reader.readString();
        if (!id || !requirement) {
            return std::nullopt;
        }
        manifest.dependencies.push_back({*id, *requirement});
    }

    const auto extensionCount = reader.readU32();
    if (!extensionCount) {
        return std::nullopt;
    }
    for (std::uint32_t i = 0; i < *extensionCount; ++i) {
        const auto extension = reader.readString();
        if (!extension) {
            return std::nullopt;
        }
        manifest.extensionPoints.push_back(*extension);
    }
    return manifest;
}

/// Extension point strings use the "category:name" convention.
std::string categoryOf(const std::string& extensionId) {
    const auto colon = extensionId.find(':');
    return colon == std::string::npos ? std::string{} : extensionId.substr(0, colon);
}

class PackageWorldImpl final : public PackageWorld {
public:
    PackageWorldImpl(platform::IFileSystem& fileSystem,
                     serialization::ISerializationBackend& storage)
        : fileSystem_(fileSystem), storage_(storage) {}

    // PackageWorld

    std::size_t discoverPackages(const std::filesystem::path& packagesRoot) override {
        std::size_t found = 0;
        for (const auto& entry : fileSystem_.list(packagesRoot)) {
            if (!fileSystem_.isDirectory(entry)) {
                continue;
            }
            auto manifest = readManifest(storage_, entry);
            if (!manifest) {
                continue;
            }
            const auto version = parseVersion(manifest->version);
            if (!version) {
                continue; // an unparseable version can never be resolved
            }
            // Several versions of one package may coexist (side-by-side
            // directories); rediscovering an exact version replaces it.
            discovered_[manifest->packageId][*version] = std::move(*manifest);
            ++found;
        }
        return found;
    }

    std::vector<PackageManifest> discoveredPackages() const override {
        std::vector<PackageManifest> result;
        for (const auto& [id, versions] : discovered_) {
            for (const auto& [version, manifest] : versions) {
                result.push_back(manifest);
            }
        }
        return result;
    }

    // IPackageResolver

    std::vector<PackageManifest> resolve(
        const std::vector<std::string>& packageReferences) override {
        // Minimal version selection: gather every requirement reaching each
        // package, pick the smallest discovered version satisfying all of
        // them, and iterate until the choice set stops changing (constraints
        // only accumulate, so this converges). References are "id" or
        // "id@requirement".
        std::map<std::string, std::vector<std::string>> requirements;
        for (const auto& reference : packageReferences) {
            const auto at = reference.find('@');
            const auto id = reference.substr(0, at);
            requirements[id].push_back(
                at == std::string::npos ? "*" : reference.substr(at + 1));
        }

        std::map<std::string, Version> selected;
        for (bool changed = true; changed;) {
            changed = false;
            for (const auto& [id, constraints] : requirements) {
                const auto versionsIt = discovered_.find(id);
                if (versionsIt == discovered_.end()) {
                    return {}; // unknown package
                }
                const Version* choice = nullptr;
                for (const auto& [version, manifest] : versionsIt->second) {
                    bool ok = true;
                    for (const auto& constraint : constraints) {
                        ok = ok && satisfies(version, constraint);
                    }
                    if (ok) {
                        choice = &version; // maps iterate ascending: minimal
                        break;
                    }
                }
                if (choice == nullptr) {
                    return {}; // conflicting requirements
                }
                const auto selectedIt = selected.find(id);
                if (selectedIt != selected.end() && selectedIt->second == *choice) {
                    continue;
                }
                selected[id] = *choice;
                changed = true;
                // The chosen manifest's dependencies join the requirement set.
                for (const auto& dependency :
                     versionsIt->second.at(*choice).dependencies) {
                    requirements[dependency.packageId].push_back(
                        dependency.versionRequirement);
                }
            }
        }

        // Topological activation order over the chosen set (cycles fail).
        std::vector<PackageManifest> order;
        std::unordered_set<std::string> done;
        std::unordered_set<std::string> inProgress;
        for (const auto& reference : packageReferences) {
            if (!visit(reference.substr(0, reference.find('@')), selected, order,
                       done, inProgress)) {
                return {};
            }
        }
        return order;
    }

    // IPackageRegistry

    PackageHandle registerPackage(const PackageManifest& manifest) override {
        const PackageHandle handle{nextId_++};
        registered_.emplace(handle.value, manifest);
        return handle;
    }

    const PackageManifest& manifest(PackageHandle package) const override {
        static const PackageManifest kEmpty{};
        const auto it = registered_.find(package.value);
        return it != registered_.end() ? it->second : kEmpty;
    }

    std::vector<PackageHandle> activePackages() const override {
        std::vector<PackageHandle> result;
        result.reserve(active_.size());
        for (const auto id : active_) {
            result.push_back(PackageHandle{id});
        }
        return result;
    }

    // IExtensionRegistry

    void registerExtension(const ExtensionPoint& extension) override {
        extensions_.push_back(extension);
    }

    std::vector<ExtensionPoint> extensionsByCategory(
        const std::string& category) const override {
        std::vector<ExtensionPoint> result;
        for (const auto& extension : extensions_) {
            if (extension.category == category) {
                result.push_back(extension);
            }
        }
        return result;
    }

    // IPackageActivationService

    bool activate(PackageHandle package) override {
        const auto it = registered_.find(package.value);
        if (it == registered_.end() || active_.contains(package.value)) {
            return false;
        }
        active_.insert(package.value);
        for (const auto& extensionId : it->second.extensionPoints) {
            registerExtension({extensionId, categoryOf(extensionId),
                               it->second.packageId});
        }
        return true;
    }

    bool deactivate(PackageHandle package) override {
        const auto it = registered_.find(package.value);
        if (it == registered_.end() || active_.erase(package.value) == 0) {
            return false;
        }
        const auto& packageId = it->second.packageId;
        std::erase_if(extensions_, [&](const ExtensionPoint& extension) {
            return extension.providerPackageId == packageId;
        });
        return true;
    }

private:
    bool visit(const std::string& packageId,
               const std::map<std::string, Version>& selected,
               std::vector<PackageManifest>& order,
               std::unordered_set<std::string>& done,
               std::unordered_set<std::string>& inProgress) {
        if (done.contains(packageId)) {
            return true;
        }
        if (inProgress.contains(packageId)) {
            return false; // cycle
        }
        const auto selectedIt = selected.find(packageId);
        if (selectedIt == selected.end()) {
            return false;
        }
        const auto& manifest = discovered_.at(packageId).at(selectedIt->second);
        inProgress.insert(packageId);
        for (const auto& dependency : manifest.dependencies) {
            if (!visit(dependency.packageId, selected, order, done, inProgress)) {
                return false;
            }
        }
        inProgress.erase(packageId);
        done.insert(packageId);
        order.push_back(manifest);
        return true;
    }

    platform::IFileSystem& fileSystem_;
    serialization::ISerializationBackend& storage_;
    std::uint64_t nextId_ = 1;
    // id -> version -> manifest; std::map keeps versions ascending for MVS.
    std::map<std::string, std::map<Version, PackageManifest>> discovered_;
    std::unordered_map<std::uint64_t, PackageManifest> registered_;
    std::unordered_set<std::uint64_t> active_;
    std::vector<ExtensionPoint> extensions_;
};

} // namespace

std::unique_ptr<PackageWorld> createPackageWorld(
    platform::IFileSystem& fileSystem, serialization::ISerializationBackend& storage) {
    return std::make_unique<PackageWorldImpl>(fileSystem, storage);
}

std::uint64_t manifestChecksum(const PackageManifest& manifest) {
    serialization::ByteWriter writer;
    writer.writeString(manifest.packageId);
    writer.writeString(manifest.version);
    writer.writeString(manifest.displayName);
    for (const auto& dependency : manifest.dependencies) {
        writer.writeString(dependency.packageId);
        writer.writeString(dependency.versionRequirement);
    }
    for (const auto& extension : manifest.extensionPoints) {
        writer.writeString(extension);
    }
    std::uint64_t hash = 1469598103934665603ULL; // FNV-1a
    for (const auto byte : writer.buffer()) {
        hash ^= static_cast<std::uint8_t>(byte);
        hash *= 1099511628211ULL;
    }
    return hash;
}

namespace {
constexpr const char* kLockSchemaId = "sky.package-lock";
constexpr serialization::SchemaVersion kLockSchemaVersion{1, 0};
} // namespace

bool savePackageLock(serialization::ISerializationBackend& storage,
                     const std::filesystem::path& path,
                     const std::vector<LockedPackage>& packages) {
    serialization::ByteWriter writer;
    writer.writeU32(static_cast<std::uint32_t>(packages.size()));
    for (const auto& package : packages) {
        writer.writeString(package.packageId);
        writer.writeString(package.version);
        writer.writeU64(package.manifestChecksum);
        writer.writeU32(package.active ? 1 : 0);
    }
    return storage.write(path,
                         {kLockSchemaId, kLockSchemaVersion, writer.takeBuffer()});
}

std::optional<std::vector<LockedPackage>> loadPackageLock(
    serialization::ISerializationBackend& storage,
    const std::filesystem::path& path) {
    const auto blob = storage.read(path);
    if (!blob || blob->schemaId != kLockSchemaId ||
        blob->version != kLockSchemaVersion) {
        return std::nullopt;
    }
    serialization::ByteReader reader(blob->payload);
    const auto count = reader.readU32();
    if (!count) {
        return std::nullopt;
    }
    std::vector<LockedPackage> packages;
    for (std::uint32_t i = 0; i < *count; ++i) {
        const auto id = reader.readString();
        const auto version = reader.readString();
        const auto checksum = reader.readU64();
        const auto active = reader.readU32();
        if (!id || !version || !checksum || !active) {
            return std::nullopt;
        }
        packages.push_back({*id, *version, *checksum, *active != 0});
    }
    return packages;
}

bool savePackageManifest(serialization::ISerializationBackend& storage,
                         const PackageManifest& manifest) {
    serialization::ByteWriter writer;
    writer.writeString(manifest.packageId);
    writer.writeString(manifest.version);
    writer.writeString(manifest.displayName);
    writer.writeU32(static_cast<std::uint32_t>(manifest.dependencies.size()));
    for (const auto& dependency : manifest.dependencies) {
        writer.writeString(dependency.packageId);
        writer.writeString(dependency.versionRequirement);
    }
    writer.writeU32(static_cast<std::uint32_t>(manifest.extensionPoints.size()));
    for (const auto& extension : manifest.extensionPoints) {
        writer.writeString(extension);
    }
    return storage.write(manifest.rootPath / kPackageFileName,
                         {kPackageSchemaId, kPackageSchemaVersion, writer.takeBuffer()});
}

} // namespace sky::package
