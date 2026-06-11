#pragma once

#include <QMainWindow>
#include <array>

#include "editor_commands.hpp"
#include "editor_context.hpp"
#include "sky/editor/viewport/tool_command_bus.hpp"

class QAction;
class QToolButton;
class QTimer;

namespace sky::editor {

class ConsolePanel;
class HierarchyPanel;
class InspectorPanel;
class PackagePanel;
class ProjectPanel;
class TerrainPanel;
class SceneView3D;
class ViewportWidget;

/// The editor shell: Unity-like main window with Hierarchy, Scene view,
/// Inspector, Project and Console arranged in docks around the viewport.
class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(EditorContext& context);

    /// Selects an object in every panel, as a hierarchy click would.
    void selectObjectByName(const QString& name);
    /// Enters play mode and advances the simulation (demo/verification).
    void playFrames(int frames);

private:
    void buildMenus();
    void buildToolbar();
    void buildDocks();
    void onFrameTick();
    void onSelection(quint64 objectId);
    void syncPlayButtons();
    void duplicateObject(quint64 objectId);
    void deleteObject(quint64 objectId);
    void performUndo();
    void performRedo();
    void refreshAfterHistory();
    void updateUndoActions();

    EditorContext& context_;
    HierarchyPanel* hierarchy_ = nullptr;
    InspectorPanel* inspector_ = nullptr;
    ProjectPanel* project_ = nullptr;
    PackagePanel* packagePanel_ = nullptr;
    TerrainPanel* terrainPanel_ = nullptr;
    ConsolePanel* console_ = nullptr;
    ViewportWidget* viewport_ = nullptr;
    SceneView3D* sceneView3d_ = nullptr;
    SceneView3D* gameView_ = nullptr;
    QToolButton* playButton_ = nullptr;
    QToolButton* pauseButton_ = nullptr;
    QToolButton* stopButton_ = nullptr;
    std::array<QToolButton*, 4> toolButtons_{};
    QTimer* frameTimer_ = nullptr;
    int crateCounter_ = 0;
    UndoStack undoStack_;
    std::unique_ptr<ToolCommandBus> commandBus_;
    QAction* undoAction_ = nullptr;
    QAction* redoAction_ = nullptr;
};

} // namespace sky::editor
