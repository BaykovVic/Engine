#include <filesystem>

#include "sky/package/package_world.hpp"
#include "sky/platform/platform_services.hpp"
#include "sky/serialization/backends.hpp"
#include "sky_test.hpp"

namespace {

std::filesystem::path packagesRoot() {
    return std::filesystem::temp_directory_path() / "sky_engine_tests" / "package" / "packages";
}

void writePackage(sky::serialization::ISerializationBackend& storage,
                  const std::string& id,
                  const std::vector<sky::package::PackageDependency>& deps,
                  const std::vector<std::string>& extensions) {
    sky::package::PackageManifest manifest;
    manifest.packageId = id;
    manifest.version = "1.0.0";
    manifest.displayName = id;
    manifest.rootPath = packagesRoot() / id;
    manifest.dependencies = deps;
    manifest.extensionPoints = extensions;
    CHECK(sky::package::savePackageManifest(storage, manifest));
}

void testDiscoveryAndResolution() {
    const auto fileSystem = sky::platform::createStdFileSystem();
    const auto storage = sky::serialization::createFileSerializationBackend(*fileSystem);

    // terrain-tools depends on noise-lib; standalone has no dependencies.
    writePackage(*storage, "sky.noise-lib", {}, {});
    writePackage(*storage, "sky.terrain-tools", {{"sky.noise-lib", ">=1.0"}},
                 {"importer:heightmap", "tool:terrain-brush"});
    writePackage(*storage, "sky.standalone", {}, {});

    const auto packages = sky::package::createPackageWorld(*fileSystem, *storage);
    CHECK(packages->discoverPackages(packagesRoot()) == 3);

    // Dependencies come before dependents in activation order.
    const auto resolved = packages->resolve({"sky.terrain-tools"});
    CHECK(resolved.size() == 2);
    CHECK(resolved[0].packageId == "sky.noise-lib");
    CHECK(resolved[1].packageId == "sky.terrain-tools");

    // Unknown references fail the whole resolution.
    CHECK(packages->resolve({"sky.missing"}).empty());

    // Activation registers the package's extension points by category.
    const auto handle = packages->registerPackage(resolved[1]);
    CHECK(packages->activate(handle));
    CHECK(!packages->activate(handle)); // double activation is refused
    CHECK(packages->activePackages().size() == 1);
    CHECK(packages->extensionsByCategory("importer").size() == 1);
    CHECK(packages->extensionsByCategory("tool").size() == 1);
    CHECK(packages->extensionsByCategory("importer").front().providerPackageId ==
          "sky.terrain-tools");

    // Deactivation removes the extensions again.
    CHECK(packages->deactivate(handle));
    CHECK(packages->activePackages().empty());
    CHECK(packages->extensionsByCategory("importer").empty());

    std::filesystem::remove_all(std::filesystem::temp_directory_path() /
                                "sky_engine_tests" / "package");
}

void testCycleDetection() {
    const auto fileSystem = sky::platform::createStdFileSystem();
    const auto storage = sky::serialization::createFileSerializationBackend(*fileSystem);

    writePackage(*storage, "pkg.a", {{"pkg.b", "*"}}, {});
    writePackage(*storage, "pkg.b", {{"pkg.a", "*"}}, {});

    const auto packages = sky::package::createPackageWorld(*fileSystem, *storage);
    CHECK(packages->discoverPackages(packagesRoot()) == 2);
    CHECK(packages->resolve({"pkg.a"}).empty());

    std::filesystem::remove_all(std::filesystem::temp_directory_path() /
                                "sky_engine_tests" / "package");
}

} // namespace

int main() {
    testDiscoveryAndResolution();
    testCycleDetection();
    return sky::test::summary("package_tests");
}
