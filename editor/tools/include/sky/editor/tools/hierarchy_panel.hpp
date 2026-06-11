#pragma once

#include <QTreeWidget>
#include <functional>
#include <vector>

#include "sky/object/object_world.hpp"

namespace sky::editor {

/// Unity-like Hierarchy: the object tree of the active scene with creation,
/// deletion and selection. All mutations go through the Object Model
/// contracts.
class HierarchyPanel final : public QTreeWidget {
    Q_OBJECT

public:
    using RootsProvider = std::function<std::vector<object::ObjectHandle>()>;

    HierarchyPanel(object::ObjectWorld& objects, RootsProvider rootsProvider,
                   QWidget* parent = nullptr);

    void refresh();
    [[nodiscard]] object::ObjectHandle selectedObject() const;

signals:
    void objectSelected(quint64 objectId);
    void createEmptyRequested();
    void createCrateRequested();
    void deleteRequested(quint64 objectId);

private:
    void addObjectItem(QTreeWidgetItem* parent, object::ObjectHandle object);
    void showContextMenu(const QPoint& position);

    object::ObjectWorld& objects_;
    RootsProvider rootsProvider_;
};

} // namespace sky::editor
