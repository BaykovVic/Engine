#include <filesystem>

#include "sky/asset/asset_database.hpp"
#include "sky/platform/platform_services.hpp"
#include "sky/project/project_repository.hpp"
#include "sky/serialization/backends.hpp"
#include "sky_test.hpp"

namespace {

/// Importer used by tests: accepts ".mesh" sources and reports a dependency
/// on a material next to the mesh.
class FakeMeshImporter final : public sky::asset::IAssetImporter {
public:
    bool supports(const std::filesystem::path& sourcePath) const override {
        return sourcePath.extension() == ".mesh";
    }

    std::optional<sky::asset::AssetDescriptor> import(
        const std::filesystem::path& sourcePath) override {
        sky::asset::AssetDescriptor descriptor;
        descriptor.assetType = "mesh";
        descriptor.dependencies.push_back(sky::asset::assetIdFromPath(
            std::filesystem::path(sourcePath).replace_extension(".material")));
        return descriptor;
    }
};

void testAssetDatabase() {
    const auto database = sky::asset::createAssetDatabase();
    FakeMeshImporter importer;
    database->registerImporter(importer);

    // Unsupported sources are rejected.
    CHECK(!database->importAsset("textures/wood.png").has_value());

    const auto id = database->importAsset("models/crate.mesh");
    CHECK(id.has_value());

    // Identity is stable: same path, same id, including via lookup.
    CHECK(sky::asset::assetIdFromPath("models/crate.mesh") == *id);
    CHECK(database->findBySourcePath("models/crate.mesh") == *id);
    CHECK(!database->findBySourcePath("models/other.mesh").has_value());

    const auto descriptor = database->resolve(*id);
    CHECK(descriptor.has_value());
    CHECK(descriptor->assetType == "mesh");
    CHECK(descriptor->contentVersion == 1);
    CHECK(descriptor->dependencies.size() == 1);

    // The dependency graph answers reverse queries.
    const auto materialId =
        sky::asset::assetIdFromPath("models/crate.material");
    const auto dependents = database->dependentsOf(materialId);
    CHECK(dependents.size() == 1);
    CHECK(dependents.front() == *id);

    // Reimport keeps identity and bumps the content version.
    CHECK(database->reimport(*id));
    CHECK(database->resolve(*id)->contentVersion == 2);

    database->unregisterAsset(*id);
    CHECK(!database->resolve(*id).has_value());
}

void testProjectRoundTrip() {
    const auto fileSystem = sky::platform::createStdFileSystem();
    const auto storage = sky::serialization::createFileSerializationBackend(*fileSystem);
    const auto root = std::filesystem::temp_directory_path() / "sky_engine_tests" / "asset_project" / "proj";

    {
        const auto repository = sky::project::createProjectRepository(*storage);
        sky::project::ProjectDescriptor descriptor;
        descriptor.name = "Demo";
        descriptor.rootPath = root;
        descriptor.scenePaths = {"scenes/main.scene", "scenes/menu.scene"};
        descriptor.packageReferences = {"sky.terrain-tools"};
        descriptor.settings.defaultSceneName = "main";
        descriptor.settings.enabledPackages = {"sky.terrain-tools"};

        const auto handle = repository->createProject(descriptor);
        CHECK(handle.isValid());
        CHECK(fileSystem->exists(root / sky::project::kProjectFileName));
    }

    // A fresh repository reopens the project from disk alone.
    {
        const auto repository = sky::project::createProjectRepository(*storage);
        const auto handle = repository->openProject(root);
        CHECK(handle.isValid());

        const auto& descriptor = repository->descriptor(handle);
        CHECK(descriptor.name == "Demo");
        CHECK(descriptor.rootPath == root);
        CHECK(repository->sceneList(handle).size() == 2);
        CHECK(descriptor.packageReferences ==
              std::vector<std::string>{"sky.terrain-tools"});
        CHECK(descriptor.settings.defaultSceneName == "main");

        repository->closeProject(handle);
        CHECK(!repository->descriptor(handle).name.size());
    }

    // Opening a directory without a project file fails cleanly.
    {
        const auto repository = sky::project::createProjectRepository(*storage);
        CHECK(!repository->openProject(root / "nowhere").isValid());
    }

    std::filesystem::remove_all(std::filesystem::temp_directory_path() /
                                "sky_engine_tests" / "asset_project");
}

void testSidecarGuidIdentity() {
    namespace fs = std::filesystem;
    const auto root = fs::temp_directory_path() / "sky_engine_tests" / "guid";
    fs::remove_all(root);
    fs::create_directories(root);

    const auto fileSystem = sky::platform::createStdFileSystem();
    const auto database = sky::asset::createAssetDatabase(*fileSystem);
    FakeMeshImporter importer;
    database->registerImporter(importer);

    // First import writes the sidecar with a fresh GUID.
    const auto source = root / "crate.mesh";
    fileSystem->writeAll(source, std::vector<std::byte>{std::byte{1}});
    const auto id = database->importAsset(source);
    CHECK(id.has_value());
    const auto sidecar = fs::path(source.string() + ".skymeta");
    CHECK(fs::exists(sidecar));
    // GUID identity, not the path hash.
    CHECK(*id != sky::asset::assetIdFromPath(source));

    // Re-importing the same file reuses the sidecar: identity is idempotent.
    CHECK(database->importAsset(source) == *id);

    // Rename with the sidecar travelling along: the identity survives.
    const auto renamed = root / "barrel.mesh";
    fs::rename(source, renamed);
    fs::rename(sidecar, fs::path(renamed.string() + ".skymeta"));
    CHECK(database->importAsset(renamed) == *id);
    CHECK(database->findBySourcePath(renamed) == *id);
    CHECK(database->resolve(*id)->sourcePath == renamed);

    // A file without a sidecar is a new asset.
    const auto other = root / "other.mesh";
    fileSystem->writeAll(other, std::vector<std::byte>{std::byte{2}});
    const auto otherId = database->importAsset(other);
    CHECK(otherId.has_value());
    CHECK(*otherId != *id);

    // A second database session (fresh process analog) reads the same GUID
    // back from the sidecar — identity persists across runs.
    const auto secondSession = sky::asset::createAssetDatabase(*fileSystem);
    secondSession->registerImporter(importer);
    CHECK(secondSession->importAsset(renamed) == *id);

    fs::remove_all(root);
}

} // namespace

int main() {
    testAssetDatabase();
    testProjectRoundTrip();
    testSidecarGuidIdentity();
    return sky::test::summary("asset_project_tests");
}
