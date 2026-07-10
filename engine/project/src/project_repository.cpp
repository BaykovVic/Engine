#include <unordered_map>

#include "sky/project/project_repository.hpp"
#include "sky/serialization/byte_stream.hpp"

namespace sky::project {
namespace {

constexpr const char* kProjectSchemaId = "sky.project";
constexpr serialization::SchemaVersion kProjectSchemaVersion{1, 0};

class ProjectRepositoryImpl final : public ProjectRepository {
public:
    explicit ProjectRepositoryImpl(serialization::ISerializationBackend& storage)
        : storage_(storage) {}

    // IProjectRepository

    ProjectHandle createProject(const ProjectDescriptor& descriptor) override {
        const ProjectHandle handle{nextId_++};
        projects_.emplace(handle.value, descriptor);
        if (!persist(descriptor)) {
            projects_.erase(handle.value);
            return ProjectHandle::invalid();
        }
        return handle;
    }

    ProjectHandle openProject(const std::filesystem::path& rootPath) override {
        const auto blob = storage_.read(rootPath / kProjectFileName);
        if (!blob || blob->schemaId != kProjectSchemaId ||
            blob->version != kProjectSchemaVersion) {
            return ProjectHandle::invalid();
        }

        serialization::ByteReader reader(blob->payload);
        ProjectDescriptor descriptor;
        descriptor.rootPath = rootPath;

        const auto name = reader.readString();
        const auto defaultScene = reader.readString();
        if (!name || !defaultScene) {
            return ProjectHandle::invalid();
        }
        descriptor.name = *name;
        descriptor.settings.defaultSceneName = *defaultScene;

        if (!readStringList(reader, [&](std::string value) {
                descriptor.scenePaths.emplace_back(std::move(value));
            }) ||
            !readStringList(reader, [&](std::string value) {
                descriptor.packageReferences.push_back(std::move(value));
            }) ||
            !readStringList(reader, [&](std::string value) {
                descriptor.settings.enabledPackages.push_back(std::move(value));
            })) {
            return ProjectHandle::invalid();
        }

        const ProjectHandle handle{nextId_++};
        projects_.emplace(handle.value, std::move(descriptor));
        return handle;
    }

    bool saveProject(ProjectHandle project) override {
        const auto it = projects_.find(project.value);
        return it != projects_.end() && persist(it->second);
    }

    void closeProject(ProjectHandle project) override { projects_.erase(project.value); }

    // IProjectQueryService

    const ProjectDescriptor& descriptor(ProjectHandle project) const override {
        static const ProjectDescriptor kEmpty{};
        const auto it = projects_.find(project.value);
        return it != projects_.end() ? it->second : kEmpty;
    }

    std::vector<std::filesystem::path> sceneList(ProjectHandle project) const override {
        const auto it = projects_.find(project.value);
        return it != projects_.end() ? it->second.scenePaths
                                     : std::vector<std::filesystem::path>{};
    }

private:
    bool persist(const ProjectDescriptor& descriptor) {
        serialization::ByteWriter writer;
        writer.writeString(descriptor.name);
        writer.writeString(descriptor.settings.defaultSceneName);

        writer.writeU32(static_cast<std::uint32_t>(descriptor.scenePaths.size()));
        for (const auto& path : descriptor.scenePaths) {
            writer.writeString(path.generic_string());
        }
        writer.writeU32(static_cast<std::uint32_t>(descriptor.packageReferences.size()));
        for (const auto& reference : descriptor.packageReferences) {
            writer.writeString(reference);
        }
        writer.writeU32(
            static_cast<std::uint32_t>(descriptor.settings.enabledPackages.size()));
        for (const auto& package : descriptor.settings.enabledPackages) {
            writer.writeString(package);
        }

        return storage_.write(descriptor.rootPath / kProjectFileName,
                              {kProjectSchemaId, kProjectSchemaVersion,
                               writer.takeBuffer()});
    }

    template <typename Consumer>
    static bool readStringList(serialization::ByteReader& reader, Consumer consumer) {
        const auto count = reader.readU32();
        if (!count) {
            return false;
        }
        for (std::uint32_t i = 0; i < *count; ++i) {
            auto value = reader.readString();
            if (!value) {
                return false;
            }
            consumer(std::move(*value));
        }
        return true;
    }

    serialization::ISerializationBackend& storage_;
    std::uint64_t nextId_ = 1;
    std::unordered_map<std::uint64_t, ProjectDescriptor> projects_;
};

} // namespace

std::unique_ptr<ProjectRepository> createProjectRepository(
    serialization::ISerializationBackend& storage) {
    return std::make_unique<ProjectRepositoryImpl>(storage);
}

} // namespace sky::project
