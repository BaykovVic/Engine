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
    const auto root = std::filesystem::temp_directory_path() / "sky_engine_tests" / "proj";

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
                                "sky_engine_tests");
}

} // namespace

int main() {
    testAssetDatabase();
    testProjectRoundTrip();
    return sky::test::summary("asset_project_tests");
}
