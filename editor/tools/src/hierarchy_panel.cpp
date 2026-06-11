#include "sky/editor/tools/hierarchy_panel.hpp"

#include <QMenu>

namespace sky::editor {

HierarchyPanel::HierarchyPanel(object::ObjectWorld& objects, RootsProvider rootsProvider,
                               QWidget* parent)
    : QTreeWidget(parent), objects_(objects), rootsProvider_(std::move(rootsProvider)) {
    setHeaderHidden(true);
    setObjectName("hierarchyTree");
    setContextMenuPolicy(Qt::CustomContextMenu);
    setExpandsOnDoubleClick(false);

    connect(this, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem* current, QTreeWidgetItem*) {
                emit objectSelected(
                    current != nullptr ? current->data(0, Qt::UserRole).toULongLong() : 0);
            });
    connect(this, &QWidget::customContextMenuRequested, this,
            &HierarchyPanel::showContextMenu);
}

void HierarchyPanel::refresh() {
    const auto selected = selectedObject();
    clear();
    for (const auto root : rootsProvider_()) {
        addObjectItem(nullptr, root);
    }
    expandAll();

    if (selected.isValid()) {
        for (QTreeWidgetItemIterator it(this); *it != nullptr; ++it) {
            if ((*it)->data(0, Qt::UserRole).toULongLong() == selected.value) {
                setCurrentItem(*it);
                break;
            }
        }
    }
}

object::ObjectHandle HierarchyPanel::selectedObject() const {
    const auto* item = currentItem();
    return item != nullptr
               ? object::ObjectHandle{item->data(0, Qt::UserRole).toULongLong()}
               : object::ObjectHandle::invalid();
}

void HierarchyPanel::addObjectItem(QTreeWidgetItem* parent, object::ObjectHandle object) {
    if (!objects_.exists(object)) {
        return;
    }
    auto* item = parent != nullptr ? new QTreeWidgetItem(parent)
                                   : new QTreeWidgetItem(this);
    item->setText(0, QString::fromStdString(objects_.nameOf(object)));
    item->setData(0, Qt::UserRole, static_cast<qulonglong>(object.value));
    for (const auto child : objects_.childrenOf(object)) {
        addObjectItem(item, child);
    }
}

void HierarchyPanel::showContextMenu(const QPoint& position) {
    QMenu menu(this);
    menu.addAction(tr("Create Empty"), this,
                   [this] { emit createEmptyRequested(); });
    menu.addAction(tr("Create Cube"), this,
                   [this] { emit createCrateRequested(); });
    const auto selected = selectedObject();
    if (selected.isValid()) {
        menu.addSeparator();
        menu.addAction(tr("Delete"), this,
                       [this, selected] { emit deleteRequested(selected.value); });
    }
    menu.exec(viewport()->mapToGlobal(position));
}

} // namespace sky::editor
