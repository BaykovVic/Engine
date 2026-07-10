#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "sky/core/handle.hpp"

namespace sky::package {

struct PackageTag {};
using PackageHandle = core::Handle<PackageTag>;

struct PackageDependency {
    std::string packageId;
    std::string versionRequirement;
};

/// Local package manifest. The Package System is strictly local-first: no
/// external registry, no SaaS.
struct PackageManifest {
    std::string packageId;
    std::string version;
    std::string displayName;
    std::filesystem::path rootPath;
    std::vector<PackageDependency> dependencies;
    std::vector<std::string> extensionPoints;
};

/// A capability a package contributes to the engine or editor (component
/// types, importers, tools, generators).
struct ExtensionPoint {
    std::string id;
    std::string category;
    std::string providerPackageId;
};

/// Package System contract: discovery and dependency resolution over local
/// packages referenced by the Project Model.
class IPackageResolver {
public:
    virtual ~IPackageResolver() = default;

    /// Resolves the package graph for the given references; returns packages
    /// in activation (topological) order.
    virtual std::vector<PackageManifest> resolve(
        const std::vector<std::string>& packageReferences) = 0;
};

/// Package System contract: registry of discovered and activated packages.
class IPackageRegistry {
public:
    virtual ~IPackageRegistry() = default;

    virtual PackageHandle registerPackage(const PackageManifest& manifest) = 0;
    [[nodiscard]] virtual const PackageManifest& manifest(PackageHandle package) const = 0;
    [[nodiscard]] virtual std::vector<PackageHandle> activePackages() const = 0;
};

/// Package System contract: extension points contributed by active packages.
class IExtensionRegistry {
public:
    virtual ~IExtensionRegistry() = default;

    virtual void registerExtension(const ExtensionPoint& extension) = 0;
    [[nodiscard]] virtual std::vector<ExtensionPoint> extensionsByCategory(
        const std::string& category) const = 0;
};

/// Package System contract: package lifecycle (activate / deactivate).
class IPackageActivationService {
public:
    virtual ~IPackageActivationService() = default;

    virtual bool activate(PackageHandle package) = 0;
    virtual bool deactivate(PackageHandle package) = 0;
};

} // namespace sky::package
