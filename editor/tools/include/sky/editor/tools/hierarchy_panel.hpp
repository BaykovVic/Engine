#pragma once

#include <QTreeWidget>
#include <functional>
#include <vector>

#include "sky/object/object_world.hpp"

namespace sky::editor {

/// Unity-like Hierarchy: the object tree of the active scene with creation,
/// deletion, double-click rename, drag-and-drop reparenting and selection.
/// All mutations go through the Object Model contracts.
class HierarchyPanel final : public QTreeWidget {
    Q_OBJECT

public:
    using RootsProvider = std::function<std::vector<object::ObjectHandle>()>;

    HierarchyPanel(object::ObjectWorld& objects, RootsProvider rootsProvider,
                   QWidget* parent = nullptr);

    void refresh();
    [[nodiscard]] object::ObjectHandle selectedObject() const;
    void selectObject(object::ObjectHandle object);

signals:
    void objectSelected(quint64 objectId);
    void createEmptyRequested();
    void createCrateRequested();
    void deleteRequested(quint64 objectId);
    void duplicateRequested(quint64 objectId);
    /// newParentId == 0 means "make it a scene root".
    void reparentRequested(quint64 objectId, quint64 newParentId);
    /// Emitted after a successful in-place rename, with both names.
    void objectRenamed(quint64 objectId, QString oldName, QString newName);

protected:
    void dropEvent(QDropEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void addObjectItem(QTreeWidgetItem* parent, object::ObjectHandle object);
    void showContextMenu(const QPoint& position);

    object::ObjectWorld& objects_;
    RootsProvider rootsProvider_;
    bool refreshing_ = false;
};

} // namespace sky::editor
