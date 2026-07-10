#include "sky/editor/tools/project_panel.hpp"

#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

namespace sky::editor {

ProjectPanel::ProjectPanel(platform::IVirtualFileSystem& vfs, QWidget* parent)
    : QWidget(parent), vfs_(vfs) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    pathLabel_ = new QLabel(this);
    pathLabel_->setObjectName("projectPathLabel");
    layout->addWidget(pathLabel_);

    entries_ = new QListWidget(this);
    entries_->setObjectName("projectEntries");
    entries_->setViewMode(QListView::IconMode);
    entries_->setIconSize(QSize(48, 48));
    entries_->setGridSize(QSize(96, 80));
    entries_->setResizeMode(QListView::Adjust);
    entries_->setMovement(QListView::Static);
    entries_->setWordWrap(true);
    connect(entries_, &QListWidget::itemActivated, this, &ProjectPanel::onItemActivated);
    connect(entries_, &QListWidget::itemDoubleClicked, this,
            &ProjectPanel::onItemActivated);
    layout->addWidget(entries_, 1);

    navigateTo("project://");
}

void ProjectPanel::navigateTo(const QString& virtualDir) {
    currentDir_ = virtualDir;
    refresh();
}

void ProjectPanel::refresh() {
    pathLabel_->setText(currentDir_);
    entries_->clear();

    const auto path = platform::VfsPath::parse(currentDir_.toStdString());
    if (path && !path->relative.empty()) {
        auto* up = new QListWidgetItem(QString::fromUtf8("⬑ .."), entries_);
        up->setData(Qt::UserRole, QStringLiteral(".."));
    }

    for (const auto& entry : vfs_.list(currentDir_.toStdString())) {
        const bool isDirectory = !entry.empty() && entry.back() == '/';
        const auto name =
            QString::fromStdString(isDirectory ? entry.substr(0, entry.size() - 1) : entry);
        const auto glyph = isDirectory ? QString::fromUtf8("\U0001F4C1")
                                       : QString::fromUtf8("\U0001F4C4");
        auto* item = new QListWidgetItem(glyph + "\n" + name, entries_);
        item->setData(Qt::UserRole, QString::fromStdString(entry));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignTop);
    }
}

void ProjectPanel::onItemActivated(QListWidgetItem* item) {
    const auto entry = item->data(Qt::UserRole).toString();
    auto path = platform::VfsPath::parse(currentDir_.toStdString());
    if (!path) {
        return;
    }
    if (entry == "..") {
        const auto slash = path->relative.rfind('/');
        path->relative = slash == std::string::npos ? "" : path->relative.substr(0, slash);
        navigateTo(QString::fromStdString(path->toString()));
        return;
    }
    if (entry.endsWith('/')) {
        const auto directory = entry.left(entry.size() - 1).toStdString();
        path->relative = path->relative.empty() ? directory
                                                : path->relative + "/" + directory;
        navigateTo(QString::fromStdString(path->toString()));
    }
}

} // namespace sky::editor
