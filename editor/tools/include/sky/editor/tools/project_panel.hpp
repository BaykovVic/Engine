#pragma once

#include <QWidget>

#include "sky/platform/virtual_file_system.hpp"

class QLabel;
class QListWidget;
class QListWidgetItem;

namespace sky::editor {

/// Unity-like Project window: browses the engine's virtual file system
/// (project://, assets://, packages://...) rather than raw OS paths.
class ProjectPanel final : public QWidget {
    Q_OBJECT

public:
    explicit ProjectPanel(platform::IVirtualFileSystem& vfs, QWidget* parent = nullptr);

    void navigateTo(const QString& virtualDir);
    void refresh();

private:
    void onItemActivated(QListWidgetItem* item);

    platform::IVirtualFileSystem& vfs_;
    QString currentDir_;
    QLabel* pathLabel_ = nullptr;
    QListWidget* entries_ = nullptr;
};

} // namespace sky::editor
