#pragma once

#include <filesystem>
#include <string>

#include "sky/project/project_model.hpp"

namespace sky::editor {

/// State of one editor working session: the opened project and the dock
/// layout. The shell owns UI state only — never the scene or runtime state.
struct EditorSession {
    project::ProjectHandle project;
    std::string layoutName;
};

/// Serialized arrangement of editor dock panels.
struct DockLayout {
    std::string name;
    std::string serializedState;
};

/// Editor Shell contract: main window, panels, docking and the editor
/// session lifecycle. Implementations live in the Qt5 editor layer.
class IEditorShell {
public:
    virtual ~IEditorShell() = default;

    virtual bool openProject(const std::filesystem::path& projectRoot) = 0;
    virtual void closeProject() = 0;
    virtual void applyLayout(const DockLayout& layout) = 0;
    [[nodiscard]] virtual DockLayout currentLayout() const = 0;
};

/// Editor Shell contract: access to the active editor session.
class IEditorSession {
public:
    virtual ~IEditorSession() = default;

    [[nodiscard]] virtual const EditorSession& current() const = 0;
    [[nodiscard]] virtual bool hasOpenProject() const = 0;
};

} // namespace sky::editor
