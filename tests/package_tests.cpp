#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "sky/package/package_installer.hpp"
#include "sky/package/package_lock.hpp"
#include "sky/package/package_world.hpp"
#include "sky/package/semver.hpp"
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

/// Like writePackage, with an explicit version and directory — several
/// versions of one package live side by side in distinct directories.
void writePackageVersion(sky::serialization::ISerializationBackend& storage,
                         const std::string& id, const std::string& version,
                         const std::string& dirName,
                         const std::vector<sky::package::PackageDependency>& deps) {
    sky::package::PackageManifest manifest;
    manifest.packageId = id;
    manifest.version = version;
    manifest.displayName = id;
    manifest.rootPath = packagesRoot() / dirName;
    manifest.dependencies = deps;
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

void testSemver() {
    using sky::package::parseVersion;
    using sky::package::satisfies;
    using sky::package::Version;

    CHECK((parseVersion("1.2.3") == Version{1, 2, 3}));
    CHECK((parseVersion("1.2") == Version{1, 2, 0}));
    CHECK((parseVersion("2") == Version{2, 0, 0}));
    CHECK(!parseVersion("").has_value());
    CHECK(!parseVersion("1.2.3-beta").has_value());
    CHECK(!parseVersion("abc").has_value());

    CHECK((satisfies({1, 2, 3}, "*")));
    CHECK((satisfies({1, 2, 3}, "")));
    CHECK((satisfies({1, 2, 3}, ">=1.2")));
    CHECK((satisfies({1, 2, 3}, ">=1.2.3")));
    CHECK((!satisfies({1, 2, 2}, ">=1.2.3")));
    CHECK((satisfies({1, 9, 0}, "^1.2")));
    CHECK((!satisfies({2, 0, 0}, "^1.2"))); // major bump breaks caret
    CHECK((!satisfies({1, 1, 0}, "^1.2")));
    CHECK((satisfies({1, 2, 3}, "1.2.3")));
    CHECK((!satisfies({1, 2, 4}, "1.2.3")));
    CHECK((satisfies({1, 2, 9}, "1.2"))); // written precision pins major.minor
    CHECK((satisfies({1, 9, 9}, "1")));
    CHECK((!satisfies({2, 0, 0}, "1")));
    CHECK((!satisfies({1, 0, 0}, "garbage"))); // malformed matches nothing
}

void testMinimalVersionSelection() {
    const auto fileSystem = sky::platform::createStdFileSystem();
    const auto storage = sky::serialization::createFileSerializationBackend(*fileSystem);

    // Three versions of one library, two consumers with different constraints.
    writePackageVersion(*storage, "pkg.lib", "1.0.0", "pkg.lib-1.0.0", {});
    writePackageVersion(*storage, "pkg.lib", "1.5.0", "pkg.lib-1.5.0", {});
    writePackageVersion(*storage, "pkg.lib", "2.0.0", "pkg.lib-2.0.0", {});
    writePackageVersion(*storage, "pkg.app", "1.0.0", "pkg.app",
                        {{"pkg.lib", ">=1.2"}});
    writePackageVersion(*storage, "pkg.tool", "1.0.0", "pkg.tool",
                        {{"pkg.lib", "^1"}});

    const auto packages = sky::package::createPackageWorld(*fileSystem, *storage);
    CHECK(packages->discoverPackages(packagesRoot()) == 5);

    // MVS: the minimal version satisfying both >=1.2 and ^1 is 1.5.0.
    const auto resolved = packages->resolve({"pkg.app", "pkg.tool"});
    CHECK(resolved.size() == 3);
    bool libSeen = false;
    for (std::size_t i = 0; i < resolved.size(); ++i) {
        if (resolved[i].packageId == "pkg.lib") {
            libSeen = true;
            CHECK(resolved[i].version == "1.5.0");
            CHECK(i == 0); // the dependency precedes both dependents
        }
    }
    CHECK(libSeen);

    // A direct constrained reference selects within the constraint.
    const auto v2 = packages->resolve({"pkg.lib@^2"});
    CHECK(v2.size() == 1);
    CHECK(v2.front().version == "2.0.0");

    // Conflicting requirements (^1 vs >=2) fail the resolution entirely.
    writePackageVersion(*storage, "pkg.clash", "1.0.0", "pkg.clash",
                        {{"pkg.lib", ">=2"}});
    CHECK(packages->discoverPackages(packagesRoot()) == 6);
    CHECK(packages->resolve({"pkg.clash", "pkg.tool"}).empty());

    // An unsatisfiable direct reference fails too.
    CHECK(packages->resolve({"pkg.lib@>=3"}).empty());

    std::filesystem::remove_all(std::filesystem::temp_directory_path() /
                                "sky_engine_tests" / "package");
}

void testLockRoundTrip() {
    const auto fileSystem = sky::platform::createStdFileSystem();
    const auto storage = sky::serialization::createFileSerializationBackend(*fileSystem);
    const auto lockPath = std::filesystem::temp_directory_path() /
                          "sky_engine_tests" / "package" /
                          sky::package::kPackageLockFileName;

    sky::package::PackageManifest manifest;
    manifest.packageId = "pkg.locked";
    manifest.version = "1.4.0";
    manifest.dependencies = {{"pkg.lib", ">=1.0"}};
    const auto checksum = sky::package::manifestChecksum(manifest);
    // The checksum is stable, and it moves when the manifest changes.
    CHECK(checksum == sky::package::manifestChecksum(manifest));
    auto changed = manifest;
    changed.version = "1.5.0";
    CHECK(sky::package::manifestChecksum(changed) != checksum);

    const std::vector<sky::package::LockedPackage> locked = {
        {"pkg.locked", "1.4.0", checksum, true},
        {"pkg.lib", "1.0.0", 12345, false},
    };
    CHECK(sky::package::savePackageLock(*storage, lockPath, locked));
    const auto loaded = sky::package::loadPackageLock(*storage, lockPath);
    CHECK(loaded.has_value());
    CHECK(loaded->size() == 2);
    CHECK((*loaded)[0].packageId == "pkg.locked");
    CHECK((*loaded)[0].version == "1.4.0");
    CHECK((*loaded)[0].manifestChecksum == checksum);
    CHECK((*loaded)[0].active);
    CHECK(!(*loaded)[1].active);

    // A missing file loads as nothing, not as an empty lock.
    CHECK(!sky::package::loadPackageLock(*storage, lockPath.parent_path() / "nope")
               .has_value());

    std::filesystem::remove_all(std::filesystem::temp_directory_path() /
                                "sky_engine_tests" / "package");
}

void testInstaller() {
    // Shells out to tar and git; skip cleanly where they are absent.
    if (std::system("tar --version > /dev/null 2>&1") != 0 ||
        std::system("git --version > /dev/null 2>&1") != 0) {
        std::puts("package_tests: no tar/git, skipping installer case");
        return;
    }
    const auto fileSystem = sky::platform::createStdFileSystem();
    const auto storage = sky::serialization::createFileSerializationBackend(*fileSystem);
    const auto base = std::filesystem::temp_directory_path() / "sky_engine_tests" /
                      "package" / "installer";
    const auto cache = base / "cache";
    const auto project = base / "Packages";
    std::filesystem::create_directories(project);

    // Source package: a directory with a manifest and a payload file.
    sky::package::PackageManifest manifest;
    manifest.packageId = "pkg.dist";
    manifest.version = "1.1.0";
    manifest.displayName = "Distributed";
    manifest.rootPath = base / "src" / "pkg.dist";
    CHECK(sky::package::savePackageManifest(*storage, manifest));
    {
        std::ofstream payload(manifest.rootPath / "readme.txt");
        payload << "hello";
    }

    sky::package::PackageInstaller installer(*storage, cache);

    // 1) Directory install: lands in the cache and in the project.
    const auto fromDir = installer.install(manifest.rootPath.string(), project);
    CHECK(fromDir.has_value());
    CHECK(fromDir->packageId == "pkg.dist");
    CHECK(fromDir->rootPath == project / "pkg.dist-1.1.0");
    CHECK(std::filesystem::exists(project / "pkg.dist-1.1.0" / "readme.txt"));
    CHECK(std::filesystem::exists(cache / "pkg.dist" / "1.1.0" / "readme.txt"));

    // The installed copy is discoverable and resolvable.
    const auto packages = sky::package::createPackageWorld(*fileSystem, *storage);
    CHECK(packages->discoverPackages(project) == 1);
    CHECK(packages->resolve({"pkg.dist@^1"}).size() == 1);

    // 2) Tarball install (a second version, packed with tar).
    auto v2 = manifest;
    v2.version = "2.0.0";
    v2.rootPath = base / "src" / "pkg.dist-2";
    CHECK(sky::package::savePackageManifest(*storage, v2));
    const auto tarball = base / "pkg.dist-2.tar.gz";
    CHECK(std::system(("tar -czf \"" + tarball.string() + "\" -C \"" +
                       v2.rootPath.parent_path().string() + "\" pkg.dist-2 " +
                       "> /dev/null 2>&1")
                          .c_str()) == 0);
    const auto fromTar = installer.install(tarball.string(), project);
    CHECK(fromTar.has_value());
    CHECK(fromTar->version == "2.0.0");
    CHECK(std::filesystem::exists(project / "pkg.dist-2.0.0"));
    CHECK(packages->discoverPackages(project) == 2); // both versions side by side

    // 3) Git install from a local bare repository, pinned to a tag.
    const auto work = base / "gitwork";
    auto v3 = manifest;
    v3.version = "3.0.0";
    v3.rootPath = work;
    CHECK(sky::package::savePackageManifest(*storage, v3));
    const auto bare = base / "pkg.dist.git";
    const std::string gitBase =
        "git -C \"" + work.string() + "\" -c user.email=t@t -c user.name=t ";
    CHECK(std::system((gitBase + "init -q").c_str()) == 0);
    CHECK(std::system((gitBase + "add .").c_str()) == 0);
    CHECK(std::system((gitBase + "commit -q -m pkg").c_str()) == 0);
    CHECK(std::system((gitBase + "tag v3.0.0").c_str()) == 0);
    CHECK(std::system(("git clone -q --bare \"" + work.string() + "\" \"" +
                       bare.string() + "\" > /dev/null 2>&1")
                          .c_str()) == 0);
    const auto fromGit = installer.install(bare.string() + "#v3.0.0", project);
    CHECK(fromGit.has_value());
    CHECK(fromGit->version == "3.0.0");
    CHECK(!std::filesystem::exists(project / "pkg.dist-3.0.0" / ".git"));

    // A bogus source fails without touching the project.
    CHECK(!installer.install("/nonexistent/thing.tar.gz", project).has_value());
    CHECK(!installer.install(base.string(), project).has_value()); // no manifest

    std::filesystem::remove_all(std::filesystem::temp_directory_path() /
                                "sky_engine_tests" / "package");
}

} // namespace

int main() {
    testSemver();
    testDiscoveryAndResolution();
    testCycleDetection();
    testMinimalVersionSelection();
    testLockRoundTrip();
    testInstaller();
    return sky::test::summary("package_tests");
}
