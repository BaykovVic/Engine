#pragma once

#include <memory>

#include "sky/project/project_model.hpp"
#include "sky/serialization/serialization.hpp"

namespace sky::project {

/// Name of the project descriptor file inside the project root.
inline constexpr const char* kProjectFileName = "project.skyproj";

/// Implementation of the Project Model backed by Serialization and
/// Persistence: the project descriptor round-trips through the local
/// project directory.
class ProjectRepository : public IProjectRepository, public IProjectQueryService {
public:
    ~ProjectRepository() override = default;
};

std::unique_ptr<ProjectRepository> createProjectRepository(
    serialization::ISerializationBackend& storage);

} // namespace sky::project
