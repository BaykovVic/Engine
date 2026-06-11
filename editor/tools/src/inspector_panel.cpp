#include "sky/editor/tools/inspector_panel.hpp"

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

namespace sky::editor {
namespace {

QHBoxLayout* vectorRow(const std::array<QDoubleSpinBox*, 3>& boxes) {
    auto* row = new QHBoxLayout;
    const char* axes[] = {"X", "Y", "Z"};
    for (int i = 0; i < 3; ++i) {
        auto* label = new QLabel(axes[i]);
        label->setObjectName("axisLabel");
        row->addWidget(label);
        row->addWidget(boxes[i], 1);
    }
    return row;
}

} // namespace

InspectorPanel::InspectorPanel(object::ObjectWorld& objects,
                               component::ComponentWorld& components, QWidget* parent)
    : QWidget(parent), objects_(objects), components_(components) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    content_ = new QWidget(this);
    outer->addWidget(content_);
    outer->addStretch(1);

    auto* layout = new QVBoxLayout(content_);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    nameEdit_ = new QLineEdit(content_);
    layout->addWidget(nameEdit_);

    auto* transformBox = new QGroupBox(tr("Transform"), content_);
    auto* form = new QFormLayout(transformBox);
    for (auto& boxes : {std::ref(position_), std::ref(scale_)}) {
        for (auto& box : boxes.get()) {
            box = new QDoubleSpinBox(transformBox);
            box->setRange(-100000.0, 100000.0);
            box->setDecimals(3);
            box->setSingleStep(0.1);
            box->setButtonSymbols(QAbstractSpinBox::NoButtons);
            connect(box, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                    [this](double) { applyTransformFromUi(); });
            // Live edits above; one undoable step per finished edit below.
            connect(box, &QDoubleSpinBox::editingFinished, this, [this] {
                if (updatingUi_ || !objects_.exists(current_)) {
                    return;
                }
                const auto after = objects_.localTransform(current_);
                if (after != editBaseline_) {
                    emit transformCommitted(current_.value, editBaseline_, after);
                    editBaseline_ = after;
                }
            });
        }
    }
    rotationLabel_ = new QLabel("0.000, 0.000, 0.000, 1.000", transformBox);
    form->addRow(tr("Position"), vectorRow(position_));
    form->addRow(tr("Rotation"), rotationLabel_);
    form->addRow(tr("Scale"), vectorRow(scale_));
    layout->addWidget(transformBox);

    auto* componentsBox = new QGroupBox(tr("Components"), content_);
    componentList_ = new QVBoxLayout(componentsBox);
    componentList_->setSpacing(4);
    layout->addWidget(componentsBox);

    addComponentButton_ = new QPushButton(tr("Add Component"), content_);
    addComponentButton_->setObjectName("addComponentButton");
    connect(addComponentButton_, &QPushButton::clicked, this,
            &InspectorPanel::showAddComponentMenu);
    layout->addWidget(addComponentButton_);

    setObject(object::ObjectHandle::invalid());
}

void InspectorPanel::setObject(object::ObjectHandle object) {
    current_ = object;
    const bool valid = objects_.exists(object);
    content_->setVisible(valid);
    if (!valid) {
        return;
    }
    updatingUi_ = true;
    nameEdit_->setText(QString::fromStdString(objects_.nameOf(object)));
    refreshTransform();
    rebuildComponentList();
    editBaseline_ = objects_.localTransform(object);
    updatingUi_ = false;
}

void InspectorPanel::refreshTransform() {
    if (!objects_.exists(current_)) {
        return;
    }
    const bool wasUpdating = updatingUi_;
    updatingUi_ = true;
    const auto transform = objects_.localTransform(current_);
    const float position[] = {transform.position.x, transform.position.y,
                              transform.position.z};
    const float scale[] = {transform.scale.x, transform.scale.y, transform.scale.z};
    for (int i = 0; i < 3; ++i) {
        position_[i]->setValue(position[i]);
        scale_[i]->setValue(scale[i]);
    }
    rotationLabel_->setText(QString("%1, %2, %3, %4")
                                .arg(transform.rotation.x, 0, 'f', 3)
                                .arg(transform.rotation.y, 0, 'f', 3)
                                .arg(transform.rotation.z, 0, 'f', 3)
                                .arg(transform.rotation.w, 0, 'f', 3));
    updatingUi_ = wasUpdating;
}

void InspectorPanel::applyTransformFromUi() {
    if (updatingUi_ || !objects_.exists(current_)) {
        return;
    }
    auto transform = objects_.localTransform(current_);
    transform.position = {static_cast<float>(position_[0]->value()),
                          static_cast<float>(position_[1]->value()),
                          static_cast<float>(position_[2]->value())};
    transform.scale = {static_cast<float>(scale_[0]->value()),
                       static_cast<float>(scale_[1]->value()),
                       static_cast<float>(scale_[2]->value())};
    objects_.setLocalTransform(current_, transform);
    emit objectEdited();
}

void InspectorPanel::rebuildComponentList() {
    while (auto* item = componentList_->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    for (const auto component : components_.componentsOf(current_)) {
        const auto& descriptor = components_.descriptorOf(component);
        auto* row = new QWidget;
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(4, 2, 4, 2);
        auto* name = new QLabel(QString::fromStdString(descriptor.displayName), row);
        name->setObjectName("componentName");
        rowLayout->addWidget(name, 1);
        if (descriptor.isScriptComponent) {
            auto* script =
                new QLabel(QString::fromStdString(descriptor.managedTypeName), row);
            script->setObjectName("scriptTypeName");
            rowLayout->addWidget(script);
        }
        auto* removeButton = new QPushButton("−", row);
        removeButton->setObjectName("removeComponentButton");
        removeButton->setFixedSize(20, 20);
        connect(removeButton, &QPushButton::clicked, this, [this, component] {
            components_.detach(component);
            rebuildComponentList();
            emit objectEdited();
        });
        rowLayout->addWidget(removeButton);
        componentList_->addWidget(row);
    }
}

void InspectorPanel::showAddComponentMenu() {
    QMenu menu(this);
    for (const auto& descriptor : components_.availableTypes()) {
        menu.addAction(QString::fromStdString(descriptor.displayName), this,
                       [this, typeId = descriptor.typeId] {
                           components_.attach(current_, typeId);
                           rebuildComponentList();
                           emit objectEdited();
                       });
    }
    menu.exec(addComponentButton_->mapToGlobal(
        QPoint(0, addComponentButton_->height())));
}

} // namespace sky::editor
