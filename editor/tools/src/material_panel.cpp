#include "sky/editor/tools/material_panel.hpp"

#include <QColorDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

namespace sky::editor {
namespace {

QColor toQColor(const core::Vec3& v) {
    return QColor::fromRgbF(std::clamp(v.x, 0.0f, 1.0f), std::clamp(v.y, 0.0f, 1.0f),
                            std::clamp(v.z, 0.0f, 1.0f));
}

core::Vec3 toVec3(const QColor& c) {
    return {static_cast<float>(c.redF()), static_cast<float>(c.greenF()),
            static_cast<float>(c.blueF())};
}

void paintSwatch(QPushButton* button, const core::Vec3& color) {
    button->setStyleSheet(QString("background-color: %1; border: 1px solid #232323;")
                              .arg(toQColor(color).name()));
}

} // namespace

MaterialPanel::MaterialPanel(rendering::IMaterialLibrary& materials, QWidget* parent)
    : QWidget(parent), materials_(materials) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* left = new QVBoxLayout;
    list_ = new QListWidget(this);
    list_->setMaximumWidth(180);
    left->addWidget(list_, 1);
    auto* newButton = new QPushButton(tr("New Material"), this);
    left->addWidget(newButton);
    layout->addLayout(left);

    auto* form = new QFormLayout;
    form->setContentsMargins(8, 8, 8, 8);
    nameEdit_ = new QLineEdit(this);
    colorButton_ = new QPushButton(this);
    colorButton_->setFixedSize(64, 22);
    emissiveButton_ = new QPushButton(this);
    emissiveButton_->setFixedSize(64, 22);
    roughness_ = new QSlider(Qt::Horizontal, this);
    roughness_->setRange(0, 100);
    metallic_ = new QSlider(Qt::Horizontal, this);
    metallic_->setRange(0, 100);
    textureEdit_ = new QLineEdit(this);
    textureEdit_->setPlaceholderText(tr("path/to/texture.png"));
    form->addRow(tr("Name"), nameEdit_);
    form->addRow(tr("Base Color"), colorButton_);
    form->addRow(tr("Roughness"), roughness_);
    form->addRow(tr("Metallic"), metallic_);
    form->addRow(tr("Emissive"), emissiveButton_);
    form->addRow(tr("Texture"), textureEdit_);
    layout->addLayout(form, 1);

    connect(list_, &QListWidget::currentTextChanged, this,
            [this](const QString&) { showSelected(); });
    connect(newButton, &QPushButton::clicked, this, &MaterialPanel::createMaterial);
    connect(nameEdit_, &QLineEdit::editingFinished, this, &MaterialPanel::applyEdits);
    connect(textureEdit_, &QLineEdit::editingFinished, this,
            &MaterialPanel::applyEdits);
    connect(roughness_, &QSlider::valueChanged, this,
            [this](int) { applyEdits(); });
    connect(metallic_, &QSlider::valueChanged, this, [this](int) { applyEdits(); });
    connect(colorButton_, &QPushButton::clicked, this, [this] { pickColor(false); });
    connect(emissiveButton_, &QPushButton::clicked, this,
            [this] { pickColor(true); });

    refresh();
}

void MaterialPanel::refresh() {
    updating_ = true;
    const auto selected = selectedName();
    list_->clear();
    for (const auto& desc : materials_.allMaterials()) {
        list_->addItem(QString::fromStdString(desc.name));
    }
    list_->sortItems();
    const auto matches =
        list_->findItems(selected, Qt::MatchExactly);
    list_->setCurrentItem(!matches.isEmpty() ? matches.first()
                          : list_->count() > 0 ? list_->item(0)
                                               : nullptr);
    updating_ = false;
    showSelected();
}

QString MaterialPanel::selectedName() const {
    const auto* item = list_->currentItem();
    return item != nullptr ? item->text() : QString{};
}

void MaterialPanel::showSelected() {
    const auto handle = materials_.findMaterial(selectedName().toStdString());
    setEnabled(list_->count() > 0);
    if (!handle) {
        return;
    }
    updating_ = true;
    const auto& desc = materials_.material(*handle);
    nameEdit_->setText(QString::fromStdString(desc.name));
    paintSwatch(colorButton_, desc.baseColor);
    paintSwatch(emissiveButton_, desc.emissive);
    roughness_->setValue(static_cast<int>(desc.roughness * 100.0f));
    metallic_->setValue(static_cast<int>(desc.metallic * 100.0f));
    textureEdit_->setText(QString::fromStdString(desc.texturePath));
    updating_ = false;
}

void MaterialPanel::applyEdits() {
    if (updating_) {
        return;
    }
    const auto handle = materials_.findMaterial(selectedName().toStdString());
    if (!handle) {
        return;
    }
    auto desc = materials_.material(*handle);
    desc.name = nameEdit_->text().trimmed().toStdString();
    desc.roughness = roughness_->value() / 100.0f;
    desc.metallic = metallic_->value() / 100.0f;
    desc.texturePath = textureEdit_->text().trimmed().toStdString();
    if (materials_.updateMaterial(*handle, desc)) {
        emit materialsChanged();
        if (QString::fromStdString(desc.name) != selectedName()) {
            refresh();
        }
    } else {
        showSelected(); // rejected rename: restore the editors
    }
}

void MaterialPanel::pickColor(bool emissive) {
    const auto handle = materials_.findMaterial(selectedName().toStdString());
    if (!handle) {
        return;
    }
    auto desc = materials_.material(*handle);
    const auto current = emissive ? desc.emissive : desc.baseColor;
    const auto picked = QColorDialog::getColor(toQColor(current), this);
    if (!picked.isValid()) {
        return;
    }
    (emissive ? desc.emissive : desc.baseColor) = toVec3(picked);
    if (materials_.updateMaterial(*handle, desc)) {
        showSelected();
        emit materialsChanged();
    }
}

void MaterialPanel::createMaterial() {
    rendering::MaterialDesc desc;
    desc.name = QString("Material %1").arg(++newMaterialCounter_).toStdString();
    while (!materials_.createMaterial(desc).isValid() && newMaterialCounter_ < 1000) {
        desc.name = QString("Material %1").arg(++newMaterialCounter_).toStdString();
    }
    refresh();
    emit materialsChanged();
}

} // namespace sky::editor
