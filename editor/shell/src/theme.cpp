#include "theme.hpp"

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

namespace sky::editor {

void applyDarkTheme(QApplication& app) {
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette palette;
    const QColor window(0x38, 0x38, 0x38);
    const QColor base(0x2a, 0x2a, 0x2a);
    const QColor text(0xd2, 0xd2, 0xd2);
    const QColor highlight(0x2d, 0x5c, 0x8f);
    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, QColor(0x33, 0x33, 0x33));
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, QColor(0x44, 0x44, 0x44));
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::Highlight, highlight);
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::ToolTipBase, base);
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(0x7a, 0x7a, 0x7a));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x7a, 0x7a, 0x7a));
    app.setPalette(palette);

    app.setStyleSheet(R"qss(
QMainWindow, QDialog { background: #383838; }
QMenuBar { background: #2d2d2d; color: #d2d2d2; border-bottom: 1px solid #232323; }
QMenuBar::item:selected { background: #46607c; }
QMenu { background: #2d2d2d; color: #d2d2d2; border: 1px solid #1f1f1f; }
QMenu::item:selected { background: #2d5c8f; }
QToolBar { background: #2d2d2d; border-bottom: 1px solid #232323; spacing: 4px; padding: 2px 6px; }
QToolButton { background: transparent; color: #d2d2d2; border: 1px solid transparent;
              border-radius: 3px; padding: 3px 10px; font-size: 13px; }
QToolButton:hover { background: #46484a; }
QToolButton:checked { background: #2d5c8f; border-color: #1f4263; }
QDockWidget { color: #d2d2d2; titlebar-close-icon: none; titlebar-normal-icon: none; }
QDockWidget::title { background: #2d2d2d; padding: 5px 8px; border: 1px solid #232323; }
QTreeWidget, QListWidget, QPlainTextEdit { background: #383838; color: #d2d2d2;
              border: none; outline: none; }
QTreeWidget::item { height: 22px; }
QTreeWidget::item:selected, QListWidget::item:selected { background: #2d5c8f; }
QTreeWidget::item:hover, QListWidget::item:hover { background: #424242; }
QHeaderView::section { background: #2d2d2d; color: #d2d2d2; border: none; padding: 4px; }
QGroupBox { color: #d2d2d2; border: 1px solid #2a2a2a; border-radius: 3px;
            margin-top: 12px; background: #3c3c3c; font-weight: bold; }
QGroupBox::title { subcontrol-origin: margin; left: 6px; padding: 0 3px; }
QLineEdit, QDoubleSpinBox, QSpinBox { background: #282828; color: #d2d2d2;
            border: 1px solid #232323; border-radius: 2px; padding: 2px 4px;
            selection-background-color: #2d5c8f; }
QPushButton { background: #585858; color: #e4e4e4; border: 1px solid #303030;
              border-radius: 3px; padding: 4px 12px; }
QPushButton:hover { background: #676767; }
QPushButton:pressed { background: #46607c; }
QTabWidget::pane { border: 1px solid #232323; }
QTabBar::tab { background: #2d2d2d; color: #b4b4b4; padding: 5px 14px;
               border: 1px solid #232323; border-bottom: none; }
QTabBar::tab:selected { background: #383838; color: #ffffff; }
QStatusBar { background: #2d2d2d; color: #9b9b9b; border-top: 1px solid #232323; }
QSplitter::handle { background: #232323; }
QScrollBar:vertical { background: #2d2d2d; width: 12px; }
QScrollBar::handle:vertical { background: #5a5a5a; border-radius: 4px; min-height: 24px; }
QScrollBar:horizontal { background: #2d2d2d; height: 12px; }
QScrollBar::handle:horizontal { background: #5a5a5a; border-radius: 4px; min-width: 24px; }
QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
QLabel#axisLabel { color: #9b9b9b; font-weight: bold; }
QLabel#projectPathLabel { background: #2d2d2d; color: #9b9b9b; padding: 4px 8px;
                          border-bottom: 1px solid #232323; }
QLabel#componentName { font-weight: bold; }
QLabel#scriptTypeName { color: #8ab4dd; font-style: italic; }
QWidget#consoleToolbar { background: #2d2d2d; border-bottom: 1px solid #232323; }
QPushButton#consoleClearButton, QPushButton#removeComponentButton { padding: 1px 8px; }
QPlainTextEdit#consoleOutput { font-family: "DejaVu Sans Mono", monospace; font-size: 12px; }
)qss");
}

} // namespace sky::editor
