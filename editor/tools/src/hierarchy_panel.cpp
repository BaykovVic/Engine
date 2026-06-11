#include "sky/editor/tools/hierarchy_panel.hpp"

#include <QDropEvent>
#include <QKeyEvent>
#include <QMenu>

namespace sky::editor {

HierarchyPanel::HierarchyPanel(object::ObjectWorld& objects, RootsProvider rootsProvider,
                               QWidget* parent)
    : QTreeWidget(parent), objects_(objects), rootsProvider_(std::move(rootsProvider)) {
    setHeaderHidden(true);
    setObjectName("hierarchyTree");
    setContextMenuPolicy(Qt::CustomContextMenu);
    setExpandsOnDoubleClick(false);
    setEditTriggers(QAbstractItemView::DoubleClicked |
                    QAbstractItemView::EditKeyPressed);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDefaultDropAction(Qt::MoveAction);

    connect(this, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem* current, QTreeWidgetItem*) {
                if (!refreshing_) {
                    emit objectSelected(
                        current != nullptr
                            ? current->data(0, Qt::UserRole).toULongLong()
                            : 0);
                }
            });
    connect(this, &QTreeWidget::itemChanged, this,
            [this](QTreeWidgetItem* item, int) {
                if (refreshing_ || item == nullptr) {
                    return;
                }
                const object::ObjectHandle object{
                    item->data(0, Qt::UserRole).toULongLong()};
                const auto name = item->text(0).trimmed();
                if (objects_.exists(object) && !name.isEmpty()) {
                    const auto oldName =
                        QString::fromStdString(objects_.nameOf(object));
                    if (oldName != name) {
                        objects_.renameObject(object, name.toStdString());
                        emit objectRenamed(object.value, oldName, name);
                    }
                } else {
                    refresh();
                }
            });
    connect(this, &QWidget::customContextMenuRequested, this,
            &HierarchyPanel::showContextMenu);
}

void HierarchyPanel::refresh() {
    refreshing_ = true;
    const auto selected = selectedObject();
    clear();
    for (const auto root : rootsProvider_()) {
        addObjectItem(nullptr, root);
    }
    expandAll();
    refreshing_ = false;
    if (selected.isValid()) {
        selectObject(selected);
    }
}

object::ObjectHandle HierarchyPanel::selectedObject() const {
    const auto* item = currentItem();
    return item != nullptr
               ? object::ObjectHandle{item->data(0, Qt::UserRole).toULongLong()}
               : object::ObjectHandle::invalid();
}

void HierarchyPanel::selectObject(object::ObjectHandle object) {
    refreshing_ = true;
    bool found = false;
    for (QTreeWidgetItemIterator it(this); *it != nullptr; ++it) {
        if ((*it)->data(0, Qt::UserRole).toULongLong() == object.value) {
            setCurrentItem(*it);
            found = true;
            break;
        }
    }
    if (!found) {
        setCurrentItem(nullptr);
    }
    refreshing_ = false;
}

void HierarchyPanel::dropEvent(QDropEvent* event) {
    // We translate the drop into an Object Model reparent instead of letting
    // the view shuffle items, so the tree always mirrors engine state.
    const auto dragged = selectedObject();
    event->setDropAction(Qt::IgnoreAction);
    event->accept();
    if (!dragged.isValid()) {
        return;
    }

    auto* target = itemAt(event->pos());
    quint64 newParentId = 0;
    if (target != nullptr) {
        const auto indicator = dropIndicatorPosition();
        if (indicator == QAbstractItemView::OnItem) {
            newParentId = target->data(0, Qt::UserRole).toULongLong();
        } else if (auto* parent = target->parent(); parent != nullptr) {
            newParentId = parent->data(0, Qt::UserRole).toULongLong();
        }
    }
    if (newParentId != dragged.value) {
        emit reparentRequested(dragged.value, newParentId);
    }
}

void HierarchyPanel::keyPressEvent(QKeyEvent* event) {
    const auto selected = selectedObject();
    if (selected.isValid()) {
        if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
            emit deleteRequested(selected.value);
            return;
        }
        if (event->key() == Qt::Key_D &&
            event->modifiers().testFlag(Qt::ControlModifier)) {
            emit duplicateRequested(selected.value);
            return;
        }
    }
    QTreeWidget::keyPressEvent(event);
}

void HierarchyPanel::addObjectItem(QTreeWidgetItem* parent, object::ObjectHandle object) {
    if (!objects_.exists(object)) {
        return;
    }
    auto* item = parent != nullptr ? new QTreeWidgetItem(parent)
                                   : new QTreeWidgetItem(this);
    item->setText(0, QString::fromStdString(objects_.nameOf(object)));
    item->setData(0, Qt::UserRole, static_cast<qulonglong>(object.value));
    item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemIsDragEnabled |
                   Qt::ItemIsDropEnabled);
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
        menu.addAction(tr("Rename"), this, [this] {
            if (currentItem() != nullptr) {
                editItem(currentItem(), 0);
            }
        });
        menu.addAction(tr("Duplicate\tCtrl+D"), this,
                       [this, selected] { emit duplicateRequested(selected.value); });
        menu.addAction(tr("Delete\tDel"), this,
                       [this, selected] { emit deleteRequested(selected.value); });
    }
    menu.exec(viewport()->mapToGlobal(position));
}

} // namespace sky::editor
