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

MainWindow::MainWindow(EditorContext& context) : context_(context) {
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

    connect(viewport_, &ViewportWidget::objectPicked, this, &MainWindow::onSelection);

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
    editMenu->addAction(tr("Undo"))->setEnabled(false);
    editMenu->addAction(tr("Redo"))->setEnabled(false);

    auto* gameObjectMenu = menuBar()->addMenu(tr("&GameObject"));
    gameObjectMenu->addAction(tr("Create Empty"), this, [this] {
        const auto object = context_.createEmpty("GameObject");
        hierarchy_->refresh();
        onSelection(object.value);
    });
    gameObjectMenu->addAction(tr("3D Object / Cube"), this, [this] {
        const auto crate = context_.createCrate(
            QString("Cube %1").arg(++crateCounter_).toStdString(),
            {0.0f, 5.0f, 0.0f});
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

    // Transform tool group on the left, Unity-style.
    for (const auto& glyph : {tr("✥"), tr("⤢"), tr("⟳"), tr("⛶")}) {
        auto* tool = new QToolButton(toolbar);
        tool->setText(glyph);
        tool->setCheckable(true);
        toolbar->addWidget(tool);
    }

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
        context_.createEmpty("GameObject");
        hierarchy_->refresh();
    });
    connect(hierarchy_, &HierarchyPanel::createCrateRequested, this, [this] {
        context_.createCrate(QString("Cube %1").arg(++crateCounter_).toStdString(),
                             {0.0f, 5.0f, 0.0f});
        hierarchy_->refresh();
    });
    connect(hierarchy_, &HierarchyPanel::deleteRequested, this, [this](quint64 objectId) {
        context_.destroyObject(object::ObjectHandle{objectId});
        inspector_->setObject(object::ObjectHandle::invalid());
        hierarchy_->refresh();
        viewport_->update();
    });
    connect(inspector_, &InspectorPanel::objectEdited, this, [this] {
        hierarchy_->refresh();
        viewport_->update();
    });
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
