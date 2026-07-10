#include "theme.hpp"

#include <QApplication>
#include <QFont>
#include <QPalette>
#include <QStyleFactory>

namespace sky::editor {

void applyDarkTheme(QApplication& app) {
    app.setStyle(QStyleFactory::create("Fusion"));

    // A slightly larger, cleaner base font lifts the whole UI.
    QFont base = app.font();
    base.setPointSizeF(base.pointSizeF() > 0 ? base.pointSizeF() + 0.5 : 10.0);
    app.setFont(base);

    // --- Palette: deep neutral blue-grey with one confident blue accent ----
    QPalette palette;
    const QColor window(0x1b, 0x1e, 0x24);
    const QColor panel(0x22, 0x26, 0x2e);
    const QColor input(0x16, 0x18, 0x1d);
    const QColor text(0xe6, 0xe8, 0xec);
    const QColor textMuted(0x9a, 0xa1, 0xac);
    const QColor accent(0x3d, 0x82, 0xf6);
    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, input);
    palette.setColor(QPalette::AlternateBase, panel);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, panel);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::BrightText, Qt::white);
    palette.setColor(QPalette::Highlight, accent);
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::ToolTipBase, QColor(0x2a, 0x2f, 0x39));
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::PlaceholderText, textMuted);
    palette.setColor(QPalette::Link, accent);
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(0x60, 0x66, 0x70));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText,
                     QColor(0x60, 0x66, 0x70));
    palette.setColor(QPalette::Disabled, QPalette::WindowText,
                     QColor(0x60, 0x66, 0x70));
    app.setPalette(palette);

    app.setStyleSheet(R"qss(
/* ---- Surfaces ------------------------------------------------------------ */
QMainWindow, QDialog { background: #1b1e24; }
QWidget { color: #e6e8ec; }
QToolTip { background: #2a2f39; color: #e6e8ec; border: 1px solid #3a4150;
           border-radius: 5px; padding: 5px 8px; }

/* ---- Menu bar & menus ---------------------------------------------------- */
QMenuBar { background: #181b21; color: #cbd0d8; border-bottom: 1px solid #0f1116;
           padding: 3px 6px; }
QMenuBar::item { padding: 5px 11px; border-radius: 5px; background: transparent; }
QMenuBar::item:selected { background: #2a3340; color: #ffffff; }
QMenu { background: #22262e; color: #dfe2e8; border: 1px solid #11131a;
        border-radius: 8px; padding: 6px; }
QMenu::item { padding: 6px 22px 6px 14px; border-radius: 5px; }
QMenu::item:selected { background: #3d82f6; color: #ffffff; }
QMenu::separator { height: 1px; background: #30353f; margin: 5px 8px; }

/* ---- Toolbar ------------------------------------------------------------- */
QToolBar { background: #181b21; border: none; border-bottom: 1px solid #0f1116;
           spacing: 5px; padding: 6px 10px; }
QToolBar::separator { width: 1px; background: #30353f; margin: 4px 6px; }
QToolButton { background: transparent; color: #b6bcc6; border: 1px solid transparent;
              border-radius: 7px; padding: 6px; min-width: 18px; min-height: 18px; }
QToolButton:hover { background: #2b313b; color: #ffffff; }
QToolButton:checked { background: #233a63; border-color: #3d82f6; color: #ffffff; }
QToolButton#playButton:checked { background: #1f3d2a; border-color: #57c76e; }
QToolButton#layoutButton { color: #cbd0d8; padding: 6px 12px; }

/* ---- Dock panels --------------------------------------------------------- */
QDockWidget { color: #cbd0d8; titlebar-close-icon: none; titlebar-normal-icon: none; }
QDockWidget::title { background: #181b21; padding: 7px 12px; border: none;
                     border-bottom: 1px solid #0f1116; font-weight: 600;
                     text-transform: uppercase; }
QDockWidget > QWidget { background: #22262e; }

/* ---- Scene header & 2D/3D toggle ----------------------------------------- */
QWidget#sceneHeader { background: #1d2027; border-bottom: 1px solid #0f1116; }
QToolButton[class="viewModeButton"] { background: #15171c; color: #9aa1ac;
    border: 1px solid #2c313b; padding: 4px 14px; font-weight: 600;
    min-width: 26px; }
QToolButton#viewModeFirst { border-top-left-radius: 6px;
    border-bottom-left-radius: 6px; border-right: none; }
QToolButton#viewModeLast { border-top-right-radius: 6px;
    border-bottom-right-radius: 6px; }
QToolButton[class="viewModeButton"]:hover { color: #d6dae0; }
QToolButton[class="viewModeButton"]:checked { background: #2f4f86; color: #ffffff;
    border-color: #3d82f6; }

/* ---- Tabs ---------------------------------------------------------------- */
QTabWidget::pane { border: none; background: #22262e; }
QTabBar { background: transparent; }
QTabBar::tab { background: transparent; color: #8b919c; padding: 8px 18px;
               border: none; border-bottom: 2px solid transparent; margin-right: 2px; }
QTabBar::tab:hover { color: #d6dae0; }
QTabBar::tab:selected { color: #ffffff; border-bottom: 2px solid #3d82f6; }

/* ---- Trees & lists ------------------------------------------------------- */
QTreeWidget, QListWidget { background: #22262e; color: #d6dae0; border: none;
                           outline: none; padding: 4px; }
QTreeWidget::item, QListWidget::item { height: 26px; border-radius: 6px;
                                       padding: 0 4px; }
QTreeWidget::item:hover, QListWidget::item:hover { background: #2b313b; }
QTreeWidget::item:selected, QListWidget::item:selected { background: #2f4f86;
                                                         color: #ffffff; }
QTreeView::branch { background: transparent; }
QHeaderView::section { background: #1d2027; color: #9aa1ac; border: none;
                       border-bottom: 1px solid #0f1116; padding: 6px 8px;
                       font-weight: 600; }

/* ---- Group boxes (Inspector cards) --------------------------------------- */
QGroupBox { color: #e6e8ec; border: 1px solid #2c313b; border-radius: 9px;
            margin-top: 16px; padding: 10px 12px 12px 12px; background: #272c35;
            font-weight: 700; }
QGroupBox::title { subcontrol-origin: margin; left: 12px; top: 2px; padding: 0 4px;
                   color: #cdd2da; }

/* ---- Inputs -------------------------------------------------------------- */
QLineEdit, QDoubleSpinBox, QSpinBox { background: #15171c; color: #e6e8ec;
            border: 1px solid #2c313b; border-radius: 6px; padding: 5px 8px;
            selection-background-color: #3d82f6; selection-color: #ffffff; }
QLineEdit:focus, QDoubleSpinBox:focus, QSpinBox:focus { border: 1px solid #3d82f6;
            background: #181b21; }
QDoubleSpinBox::up-button, QDoubleSpinBox::down-button,
QSpinBox::up-button, QSpinBox::down-button { width: 0; border: none; }

/* ---- Buttons ------------------------------------------------------------- */
QPushButton { background: #2e3440; color: #e6e8ec; border: 1px solid #3a4150;
              border-radius: 7px; padding: 7px 16px; font-weight: 600; }
QPushButton:hover { background: #39414f; border-color: #4a5365; }
QPushButton:pressed { background: #233a63; border-color: #3d82f6; }
QPushButton:disabled { background: #23272f; color: #5b616b; border-color: #2a2f38; }
QPushButton#addComponentButton { background: #2b5bb5; border-color: #3d82f6;
                                 color: #ffffff; }
QPushButton#addComponentButton:hover { background: #356bd0; }
QPushButton#removeComponentButton, QPushButton#consoleClearButton {
    padding: 3px 9px; border-radius: 6px; }

/* ---- Sliders ------------------------------------------------------------- */
QSlider::groove:horizontal { height: 4px; background: #15171c; border-radius: 2px; }
QSlider::sub-page:horizontal { background: #3d82f6; border-radius: 2px; }
QSlider::handle:horizontal { background: #e6e8ec; width: 14px; height: 14px;
                             margin: -6px 0; border-radius: 7px; }
QSlider::handle:horizontal:hover { background: #ffffff; }

/* ---- Status bar ---------------------------------------------------------- */
QStatusBar { background: #181b21; color: #8b919c; border-top: 1px solid #0f1116; }
QStatusBar::item { border: none; }

/* ---- Splitters & scrollbars --------------------------------------------- */
QSplitter::handle { background: #0f1116; }
QScrollBar:vertical { background: transparent; width: 12px; margin: 2px; }
QScrollBar::handle:vertical { background: #3a4150; border-radius: 5px;
                              min-height: 30px; }
QScrollBar::handle:vertical:hover { background: #4a5365; }
QScrollBar:horizontal { background: transparent; height: 12px; margin: 2px; }
QScrollBar::handle:horizontal { background: #3a4150; border-radius: 5px;
                                min-width: 30px; }
QScrollBar::handle:horizontal:hover { background: #4a5365; }
QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

/* ---- Named widgets ------------------------------------------------------- */
QLabel#axisLabel { color: #8b919c; font-weight: 700; padding: 0 4px; }
QLabel#projectPathLabel { background: #1d2027; color: #9aa1ac; padding: 6px 10px;
                          border-bottom: 1px solid #0f1116; }
QLabel#componentName { font-weight: 700; color: #eef0f3; }
QLabel#scriptTypeName { color: #6fb0ff; font-style: italic; }
QWidget#consoleToolbar { background: #1d2027; border-bottom: 1px solid #0f1116; }
QPlainTextEdit#consoleOutput { background: #16181d; color: #cdd2da; border: none;
                               font-family: "DejaVu Sans Mono", monospace;
                               font-size: 12px; padding: 6px; }
)qss");
}

} // namespace sky::editor
