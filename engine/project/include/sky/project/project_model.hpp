#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "sky/core/handle.hpp"

namespace sky::project {

struct ProjectTag {};
using ProjectHandle = core::Handle<ProjectTag>;

struct ProjectSettings {
    std::string defaultSceneName;
    std::vector<std::string> enabledPackages;
};

/// Authoritative description of a local project: where it lives, which
/// scenes it registers and which packages it references.
struct ProjectDescriptor {
    std::string name;
    std::filesystem::path rootPath;
    std::vector<std::filesystem::path> scenePaths;
    std::vector<std::string> packageReferences;
    ProjectSettings settings;
};

/// Project Model contract: open/create/save of the local project. The
/// resolved project context is the entry point for all project-level state.
class IProjectRepository {
public:
    virtual ~IProjectRepository() = default;

    virtual ProjectHandle createProject(const ProjectDescriptor& descriptor) = 0;
    virtual ProjectHandle openProject(const std::filesystem::path& rootPath) = 0;
    virtual bool saveProject(ProjectHandle project) = 0;
    virtual void closeProject(ProjectHandle project) = 0;
};

/// Project Model contract: read-only queries over the opened project.
class IProjectQueryService {
public:
    virtual ~IProjectQueryService() = default;

    [[nodiscard]] virtual const ProjectDescriptor& descriptor(ProjectHandle project) const = 0;
    [[nodiscard]] virtual std::vector<std::filesystem::path> sceneList(ProjectHandle project) const = 0;
};

} // namespace sky::project
