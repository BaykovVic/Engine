#pragma once

#include <QMainWindow>

#include "editor_context.hpp"

class QToolButton;
class QTimer;

namespace sky::editor {

class ConsolePanel;
class HierarchyPanel;
class InspectorPanel;
class ProjectPanel;
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

    EditorContext& context_;
    HierarchyPanel* hierarchy_ = nullptr;
    InspectorPanel* inspector_ = nullptr;
    ProjectPanel* project_ = nullptr;
    ConsolePanel* console_ = nullptr;
    ViewportWidget* viewport_ = nullptr;
    QToolButton* playButton_ = nullptr;
    QToolButton* pauseButton_ = nullptr;
    QToolButton* stopButton_ = nullptr;
    QTimer* frameTimer_ = nullptr;
    int crateCounter_ = 0;
};

} // namespace sky::editor
