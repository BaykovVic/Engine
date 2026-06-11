#include <filesystem>
#include <string>

#include "sky/platform/platform_services.hpp"
#include "sky/platform/virtual_file_system.hpp"
#include "sky_test.hpp"

namespace {

std::filesystem::path testRoot() {
    return std::filesystem::temp_directory_path() / "sky_engine_tests";
}

std::vector<std::byte> bytes(const std::string& text) {
    std::vector<std::byte> result(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        result[i] = static_cast<std::byte>(text[i]);
    }
    return result;
}

void testPathParsing() {
    const auto path = sky::platform::VfsPath::parse("project://scenes/main.scene");
    CHECK(path.has_value());
    CHECK(path->alias == "project");
    CHECK(path->relative == "scenes/main.scene");
    CHECK(path->toString() == "project://scenes/main.scene");

    // Leading slashes and backslashes are normalized.
    CHECK(sky::platform::VfsPath::parse("assets://\\textures\\wood.png")->relative ==
          "textures/wood.png");

    // No alias, no scheme, or escape attempts are rejected.
    CHECK(!sky::platform::VfsPath::parse("no-scheme/path").has_value());
    CHECK(!sky::platform::VfsPath::parse("://x").has_value());
    CHECK(!sky::platform::VfsPath::parse("project://../../etc/passwd").has_value());
}

void testDirectoryMountAndRouting() {
    const auto fileSystem = sky::platform::createStdFileSystem();
    const auto vfs = sky::platform::createVirtualFileSystem();
    const auto projectDir = testRoot() / "vfs_project";
    fileSystem->createDirectories(projectDir);

    CHECK(vfs->mount("project",
                     sky::platform::createDirectoryMount(*fileSystem, projectDir), 0));
    CHECK(vfs->aliases() == std::vector<std::string>{"project"});

    // Unknown aliases and unparsable paths fail cleanly.
    CHECK(!vfs->exists("unknown://file.txt"));
    CHECK(!vfs->writeAll("not-a-vfs-path", bytes("x")));

    CHECK(vfs->writeAll("project://scenes/main.scene", bytes("scene-data")));
    CHECK(vfs->exists("project://scenes/main.scene"));
    CHECK(vfs->readAll("project://scenes/main.scene") == bytes("scene-data"));

    // Directory listings mark directories with a trailing slash.
    const auto rootListing = vfs->list("project://");
    CHECK(rootListing == std::vector<std::string>{"scenes/"});
    CHECK(vfs->list("project://scenes") == std::vector<std::string>{"main.scene"});

    CHECK(vfs->remove("project://scenes/main.scene"));
    CHECK(!vfs->exists("project://scenes/main.scene"));

    fileSystem->remove(testRoot());
}

void testOverlayPriorities() {
    const auto fileSystem = sky::platform::createStdFileSystem();
    const auto vfs = sky::platform::createVirtualFileSystem();
    const auto baseDir = testRoot() / "vfs_base";
    const auto patchDir = testRoot() / "vfs_patch";

    fileSystem->writeAll(baseDir / "config.txt", bytes("base"));
    fileSystem->writeAll(baseDir / "untouched.txt", bytes("base-only"));
    fileSystem->writeAll(patchDir / "config.txt", bytes("patched"));

    // The patch overlays the base read-only content (e.g. a package
    // overriding shipped defaults).
    vfs->mount("assets",
               sky::platform::createDirectoryMount(*fileSystem, baseDir, true), 0);
    vfs->mount("assets",
               sky::platform::createDirectoryMount(*fileSystem, patchDir, false), 10);

    // Shadowing: the higher-priority mount wins for files it has…
    CHECK(vfs->readAll("assets://config.txt") == bytes("patched"));
    // …and reads fall through for files it does not.
    CHECK(vfs->readAll("assets://untouched.txt") == bytes("base-only"));

    // Listings merge both mounts without duplicates.
    const auto merged = vfs->list("assets://");
    CHECK(merged == (std::vector<std::string>{"config.txt", "untouched.txt"}));

    // Writes route to the highest-priority writable mount, never to the
    // read-only base.
    CHECK(vfs->writeAll("assets://new.txt", bytes("created")));
    CHECK(fileSystem->exists(patchDir / "new.txt"));
    CHECK(!fileSystem->exists(baseDir / "new.txt"));

    // Removing a shadowed file only touches writable mounts: the read-only
    // base copy survives and becomes visible again.
    CHECK(vfs->remove("assets://config.txt"));
    CHECK(vfs->readAll("assets://config.txt") == bytes("base"));
    CHECK(!vfs->remove("assets://untouched.txt"));

    vfs->unmountAll("assets");
    CHECK(!vfs->exists("assets://config.txt"));

    fileSystem->remove(testRoot());
}

void testPakArchive() {
    const auto fileSystem = sky::platform::createStdFileSystem();
    const auto sourceDir = testRoot() / "pak_source";
    const auto pakPath = testRoot() / "content.skypak";

    fileSystem->writeAll(sourceDir / "readme.txt", bytes("hello"));
    fileSystem->writeAll(sourceDir / "textures/wood.png", bytes("png-bytes"));
    fileSystem->writeAll(sourceDir / "textures/stone.png", bytes("stone-bytes"));

    const auto packed = sky::platform::buildPakArchive(*fileSystem, sourceDir, pakPath);
    CHECK(packed == 3);
    // Packing a missing directory fails.
    CHECK(!sky::platform::buildPakArchive(*fileSystem, testRoot() / "missing", pakPath)
               .has_value());

    const auto mount = sky::platform::createPakMount(*fileSystem, pakPath);
    CHECK(mount != nullptr);
    // A corrupt file does not produce a mount.
    fileSystem->writeAll(testRoot() / "broken.skypak", bytes("garbage"));
    CHECK(sky::platform::createPakMount(*fileSystem, testRoot() / "broken.skypak") ==
          nullptr);

    const auto vfs = sky::platform::createVirtualFileSystem();
    vfs->mount("assets", mount, 0);

    CHECK(vfs->readAll("assets://readme.txt") == bytes("hello"));
    CHECK(vfs->readAll("assets://textures/wood.png") == bytes("png-bytes"));
    CHECK(!vfs->exists("assets://missing.txt"));

    // Pak listings reconstruct the directory structure.
    CHECK(vfs->list("assets://") ==
          (std::vector<std::string>{"readme.txt", "textures/"}));
    CHECK(vfs->list("assets://textures") ==
          (std::vector<std::string>{"stone.png", "wood.png"}));

    // Paks are immutable.
    CHECK(!vfs->writeAll("assets://readme.txt", bytes("nope")));
    CHECK(!vfs->remove("assets://readme.txt"));
    CHECK(vfs->readAll("assets://readme.txt") == bytes("hello"));

    fileSystem->remove(testRoot());
}

} // namespace

int main() {
    testPathParsing();
    testDirectoryMountAndRouting();
    testOverlayPriorities();
    testPakArchive();
    return sky::test::summary("vfs_tests");
}
