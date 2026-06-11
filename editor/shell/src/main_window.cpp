#include "main_window.hpp"

#include <QApplication>
#include <QDockWidget>
#include <QLabel>
#include <QMenuBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>

#include "sky/editor/tools/console_panel.hpp"
#include "sky/editor/tools/hierarchy_panel.hpp"
#include "sky/editor/tools/inspector_panel.hpp"
#include "sky/editor/tools/project_panel.hpp"
#include "viewport_widget.hpp"

namespace sky::editor {

MainWindow::MainWindow(EditorContext& context)
    : context_(context), undoStack_(context) {
    setWindowTitle(tr("Sky Engine — SampleScene"));
    resize(1500, 900);
    setDockOptions(AllowNestedDocks | AllowTabbedDocks | AnimatedDocks);

    // Central Scene view wrapped in a Unity-like tab strip.
    auto* sceneTabs = new QTabWidget(this);
    viewport_ = new ViewportWidget(context_, sceneTabs);
    sceneTabs->addTab(viewport_, tr("Scene"));
    sceneTabs->addTab(new QLabel(tr("Game view renders here in play mode."), sceneTabs),
                      tr("Game"));
    setCentralWidget(sceneTabs);

    buildDocks();
    buildMenus();
    buildToolbar();
    statusBar()->showMessage(tr("Ready"));

    connect(viewport_, &ViewportWidget::objectPicked, this, [this](quint64 objectId) {
        onSelection(objectId);
        hierarchy_->selectObject(object::ObjectHandle{objectId});
    });
    connect(viewport_, &ViewportWidget::transformEdited, this,
            [this] { inspector_->refreshTransform(); });
    connect(viewport_, &ViewportWidget::transformCommitted, this,
            [this](quint64 objectId, const core::Transform& before,
                   const core::Transform& after) {
                undoStack_.push(makeTransformCommand(object::ObjectHandle{objectId},
                                                     before, after));
            });
    connect(viewport_, &ViewportWidget::deleteRequested, this,
            &MainWindow::deleteObject);
    connect(viewport_, &ViewportWidget::duplicateRequested, this,
            &MainWindow::duplicateObject);
    connect(viewport_, &ViewportWidget::undoRequested, this, &MainWindow::performUndo);
    connect(viewport_, &ViewportWidget::redoRequested, this, &MainWindow::performRedo);

    context_.playMode->setScene(context_.activeScene);
    context_.playMode->onStateChanged([this](PlayModeState) {
        syncPlayButtons();
        viewport_->update();
    });

    frameTimer_ = new QTimer(this);
    frameTimer_->setInterval(16);
    connect(frameTimer_, &QTimer::timeout, this, &MainWindow::onFrameTick);
    frameTimer_->start();

    console_->logger().info("Editor", "Sky Engine editor started");
    console_->logger().info("Scene", "SampleScene loaded: ground, crates, camera, light");
    hierarchy_->refresh();
}

void MainWindow::selectObjectByName(const QString& name) {
    const auto matches = context_.objects->findByName(name.toStdString());
    if (!matches.empty()) {
        onSelection(matches.front().value);
        hierarchy_->refresh();
    }
}

void MainWindow::playFrames(int frames) {
    context_.playMode->play();
    for (int i = 0; i < frames; ++i) {
        context_.playMode->tickFrame(1.0 / 60.0);
    }
    viewport_->update();
    inspector_->refreshTransform();
}

void MainWindow::buildMenus() {
    auto* fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("New Scene"));
    fileMenu->addAction(tr("Open Scene…"));
    fileMenu->addAction(tr("Save Scene"), this, [this] {
        console_->logger().info("Scene", "Scene saved");
        statusBar()->showMessage(tr("Scene saved"), 2000);
    });
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Exit"), qApp, &QApplication::quit);

    auto* editMenu = menuBar()->addMenu(tr("&Edit"));
    undoAction_ = editMenu->addAction(tr("Undo"), this, &MainWindow::performUndo,
                                      QKeySequence::Undo);
    redoAction_ = editMenu->addAction(tr("Redo"), this, &MainWindow::performRedo,
                                      QKeySequence::Redo);
    undoStack_.setOnChanged([this] { updateUndoActions(); });
    updateUndoActions();

    auto* gameObjectMenu = menuBar()->addMenu(tr("&GameObject"));
    gameObjectMenu->addAction(tr("Create Empty"), this, [this] {
        const auto object = context_.createEmpty("GameObject");
        undoStack_.push(makeCreateCommand(object, "GameObject", {}, false));
        hierarchy_->refresh();
        onSelection(object.value);
    });
    gameObjectMenu->addAction(tr("3D Object / Cube"), this, [this] {
        const auto name = QString("Cube %1").arg(++crateCounter_).toStdString();
        const auto crate = context_.createCrate(name, {0.0f, 5.0f, 0.0f});
        undoStack_.push(makeCreateCommand(crate, name, {0.0f, 5.0f, 0.0f}, true));
        hierarchy_->refresh();
        onSelection(crate.value);
        console_->logger().info("Scene", "Cube created");
    });

    auto* windowMenu = menuBar()->addMenu(tr("&Window"));
    for (auto* dock : findChildren<QDockWidget*>()) {
        windowMenu->addAction(dock->toggleViewAction());
    }

    menuBar()->addMenu(tr("&Help"))->addAction(tr("About Sky Engine"));
}

void MainWindow::buildToolbar() {
    auto* toolbar = addToolBar(tr("Main"));
    toolbar->setMovable(false);

    // Transform tool group on the left, Unity-style: Q hand, W move,
    // E rotate, R scale.
    const struct {
        QString glyph;
        QString tip;
        TransformTool tool;
    } tools[] = {
        {QString::fromUtf8("✋"), tr("Hand tool (Q) — drag to pan"), TransformTool::Hand},
        {QString::fromUtf8("✥"), tr("Move tool (W)"), TransformTool::Move},
        {QString::fromUtf8("⟳"), tr("Rotate tool (E)"), TransformTool::Rotate},
        {QString::fromUtf8("⤢"), tr("Scale tool (R)"), TransformTool::Scale},
    };
    for (std::size_t i = 0; i < std::size(tools); ++i) {
        auto* button = new QToolButton(toolbar);
        button->setText(tools[i].glyph);
        button->setToolTip(tools[i].tip);
        button->setCheckable(true);
        button->setChecked(tools[i].tool == viewport_->tool());
        connect(button, &QToolButton::clicked, this,
                [this, tool = tools[i].tool] { viewport_->setTool(tool); });
        toolbar->addWidget(button);
        toolButtons_[i] = button;
    }
    connect(viewport_, &ViewportWidget::toolChanged, this, [this](int tool) {
        for (std::size_t i = 0; i < toolButtons_.size(); ++i) {
            toolButtons_[i]->setChecked(static_cast<int>(i) == tool);
        }
        statusBar()->showMessage(
            tr("ЛКМ — выделение и гизмо · ПКМ/СКМ — панорама · колесо — зум · "
               "F — кадрировать · Ctrl — привязка · Ctrl+D — дубликат · Del — удалить"));
    });

    auto* leftSpacer = new QWidget(toolbar);
    leftSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(leftSpacer);

    const auto makeButton = [&](const QString& glyph) {
        auto* button = new QToolButton(toolbar);
        button->setText(glyph);
        button->setCheckable(true);
        toolbar->addWidget(button);
        return button;
    };
    playButton_ = makeButton(QString::fromUtf8("▶"));
    pauseButton_ = makeButton(QString::fromUtf8("⏸"));
    stopButton_ = makeButton(QString::fromUtf8("⏹"));
    stopButton_->setCheckable(false);

    connect(playButton_, &QToolButton::clicked, this, [this] {
        if (context_.playMode->state() == PlayModeState::Paused) {
            context_.playMode->play();
        } else if (!context_.playMode->play()) {
            syncPlayButtons();
        } else {
            console_->logger().info("PlayMode", "Entered play mode");
        }
    });
    connect(pauseButton_, &QToolButton::clicked, this, [this] {
        if (!context_.playMode->pause()) {
            syncPlayButtons();
        }
    });
    connect(stopButton_, &QToolButton::clicked, this, [this] {
        if (context_.playMode->stop()) {
            console_->logger().info("PlayMode", "Stopped");
        }
    });

    auto* rightSpacer = new QWidget(toolbar);
    rightSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(rightSpacer);

    auto* layoutButton = new QToolButton(toolbar);
    layoutButton->setText(tr("Layout ▾"));
    toolbar->addWidget(layoutButton);

    syncPlayButtons();
}

void MainWindow::buildDocks() {
    hierarchy_ = new HierarchyPanel(*context_.objects,
                                    [this] { return context_.rootObjects(); }, this);
    auto* hierarchyDock = new QDockWidget(tr("Hierarchy"), this);
    hierarchyDock->setWidget(hierarchy_);
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);

    inspector_ = new InspectorPanel(*context_.objects, *context_.components, this);
    auto* inspectorDock = new QDockWidget(tr("Inspector"), this);
    inspectorDock->setWidget(inspector_);
    inspectorDock->setMinimumWidth(320);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);

    project_ = new ProjectPanel(*context_.vfs, this);
    auto* projectDock = new QDockWidget(tr("Project"), this);
    projectDock->setWidget(project_);
    addDockWidget(Qt::BottomDockWidgetArea, projectDock);

    console_ = new ConsolePanel(this);
    auto* consoleDock = new QDockWidget(tr("Console"), this);
    consoleDock->setWidget(console_);
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);
    tabifyDockWidget(projectDock, consoleDock);
    projectDock->raise();

    connect(hierarchy_, &HierarchyPanel::objectSelected, this, &MainWindow::onSelection);
    connect(hierarchy_, &HierarchyPanel::createEmptyRequested, this, [this] {
        const auto object = context_.createEmpty("GameObject");
        undoStack_.push(makeCreateCommand(object, "GameObject", {}, false));
        hierarchy_->refresh();
    });
    connect(hierarchy_, &HierarchyPanel::createCrateRequested, this, [this] {
        const auto name = QString("Cube %1").arg(++crateCounter_).toStdString();
        const auto crate = context_.createCrate(name, {0.0f, 5.0f, 0.0f});
        undoStack_.push(makeCreateCommand(crate, name, {0.0f, 5.0f, 0.0f}, true));
        hierarchy_->refresh();
    });
    connect(hierarchy_, &HierarchyPanel::objectRenamed, this,
            [this](quint64 objectId, const QString& oldName, const QString& newName) {
                undoStack_.push(makeRenameCommand(object::ObjectHandle{objectId},
                                                  oldName.toStdString(),
                                                  newName.toStdString()));
                inspector_->setObject(object::ObjectHandle{objectId});
            });
    connect(hierarchy_, &HierarchyPanel::deleteRequested, this,
            &MainWindow::deleteObject);
    connect(hierarchy_, &HierarchyPanel::duplicateRequested, this,
            &MainWindow::duplicateObject);
    connect(hierarchy_, &HierarchyPanel::reparentRequested, this,
            [this](quint64 objectId, quint64 newParentId) {
                const object::ObjectHandle object{objectId};
                const object::ObjectHandle newParent{newParentId};
                const auto oldParent = context_.objects->parentOf(object);
                const auto oldLocal = context_.objects->localTransform(object);
                context_.reparent(object, newParent);
                undoStack_.push(
                    makeReparentCommand(object, oldParent, newParent, oldLocal));
                hierarchy_->refresh();
                viewport_->update();
                console_->logger().info("Scene", "Object reparented");
            });
    connect(inspector_, &InspectorPanel::objectEdited, this, [this] {
        hierarchy_->refresh();
        viewport_->update();
    });
    connect(inspector_, &InspectorPanel::transformCommitted, this,
            [this](quint64 objectId, const core::Transform& before,
                   const core::Transform& after) {
                undoStack_.push(makeTransformCommand(object::ObjectHandle{objectId},
                                                     before, after));
            });
}

void MainWindow::performUndo() {
    const auto label = undoStack_.undoLabel();
    if (undoStack_.undo()) {
        console_->logger().info("Edit", "Undo: " + label);
        refreshAfterHistory();
    }
}

void MainWindow::performRedo() {
    const auto label = undoStack_.redoLabel();
    if (undoStack_.redo()) {
        console_->logger().info("Edit", "Redo: " + label);
        refreshAfterHistory();
    }
}

void MainWindow::refreshAfterHistory() {
    hierarchy_->refresh();
    const auto selected = hierarchy_->selectedObject();
    if (context_.objects->exists(selected)) {
        inspector_->setObject(selected);
        viewport_->setSelected(selected);
    } else {
        inspector_->setObject(object::ObjectHandle::invalid());
        viewport_->setSelected(object::ObjectHandle::invalid());
    }
    viewport_->update();
}

void MainWindow::updateUndoActions() {
    undoAction_->setEnabled(undoStack_.canUndo());
    redoAction_->setEnabled(undoStack_.canRedo());
    undoAction_->setText(undoStack_.canUndo()
                             ? tr("Undo %1").arg(
                                   QString::fromStdString(undoStack_.undoLabel()))
                             : tr("Undo"));
    redoAction_->setText(undoStack_.canRedo()
                             ? tr("Redo %1").arg(
                                   QString::fromStdString(undoStack_.redoLabel()))
                             : tr("Redo"));
}

void MainWindow::duplicateObject(quint64 objectId) {
    const object::ObjectHandle source{objectId};
    const auto copy = context_.duplicateObject(source);
    if (copy.isValid()) {
        undoStack_.push(makeDuplicateCommand(source, copy));
        hierarchy_->refresh();
        onSelection(copy.value);
        hierarchy_->selectObject(copy);
        console_->logger().info("Scene", "Object duplicated");
    }
}

void MainWindow::deleteObject(quint64 objectId) {
    const object::ObjectHandle object{objectId};
    if (!context_.objects->exists(object)) {
        return;
    }
    // Snapshot before destruction so undo can rebuild the subtree.
    auto command = makeDeleteCommand(context_, object);
    context_.destroyObject(object);
    undoStack_.push(std::move(command));
    inspector_->setObject(object::ObjectHandle::invalid());
    viewport_->setSelected(object::ObjectHandle::invalid());
    hierarchy_->refresh();
    viewport_->update();
}

void MainWindow::onFrameTick() {
    context_.playMode->tickFrame(1.0 / 60.0);
    if (context_.playMode->state() == PlayModeState::Playing) {
        viewport_->update();
        inspector_->refreshTransform();
    }
}

void MainWindow::onSelection(quint64 objectId) {
    const object::ObjectHandle object{objectId};
    inspector_->setObject(object);
    viewport_->setSelected(object);
}

void MainWindow::syncPlayButtons() {
    const auto state = context_.playMode->state();
    playButton_->setChecked(state == PlayModeState::Playing);
    pauseButton_->setChecked(state == PlayModeState::Paused);
    statusBar()->showMessage(state == PlayModeState::Editing ? tr("Ready")
                             : state == PlayModeState::Playing
                                 ? tr("Playing — 60 fps fixed step")
                                 : tr("Paused"));
}

} // namespace sky::editor
