#pragma once

#include <QWidget>
#include <array>

#include "sky/component/component_world.hpp"
#include "sky/object/object_world.hpp"

class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

namespace sky::editor {

/// Unity-like Inspector: name, transform and attached components of the
/// selected object, with an Add Component menu fed by the component
/// registry.
class InspectorPanel final : public QWidget {
    Q_OBJECT

public:
    InspectorPanel(object::ObjectWorld& objects, component::ComponentWorld& components,
                   QWidget* parent = nullptr);

    void setObject(object::ObjectHandle object);
    /// Re-reads the transform of the inspected object (used while playing).
    void refreshTransform();

signals:
    void objectEdited();

private:
    void applyTransformFromUi();
    void rebuildComponentList();
    void showAddComponentMenu();

    object::ObjectWorld& objects_;
    component::ComponentWorld& components_;
    object::ObjectHandle current_;
    bool updatingUi_ = false;

    QLineEdit* nameEdit_ = nullptr;
    std::array<QDoubleSpinBox*, 3> position_{};
    std::array<QDoubleSpinBox*, 3> scale_{};
    QLabel* rotationLabel_ = nullptr;
    QVBoxLayout* componentList_ = nullptr;
    QPushButton* addComponentButton_ = nullptr;
    QWidget* content_ = nullptr;
};

} // namespace sky::editor
