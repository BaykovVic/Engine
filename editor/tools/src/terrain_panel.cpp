#include "sky/editor/tools/terrain_panel.hpp"

#include <QButtonGroup>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

namespace sky::editor {

TerrainPanel::TerrainPanel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    auto* sculptBox = new QGroupBox(tr("Sculpt"), this);
    auto* sculptLayout = new QVBoxLayout(sculptBox);

    auto* buttonsRow = new QHBoxLayout;
    brushGroup_ = new QButtonGroup(this);
    brushGroup_->setExclusive(false); // manual toggle-off handling
    const struct {
        QString label;
        QString operation;
    } brushes[] = {
        {tr("Raise"), "raise"},
        {tr("Lower"), "lower"},
        {tr("Smooth"), "smooth"},
    };
    for (int i = 0; i < 3; ++i) {
        auto* button = new QPushButton(brushes[i].label, sculptBox);
        button->setCheckable(true);
        button->setProperty("operation", brushes[i].operation);
        brushGroup_->addButton(button, i);
        buttonsRow->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, button] {
            // Radio behaviour with toggle-off: only one brush stays down.
            for (auto* other : brushGroup_->buttons()) {
                if (other != button) {
                    other->setChecked(false);
                }
            }
            emitBrush();
        });
    }
    sculptLayout->addLayout(buttonsRow);

    auto* sliders = new QFormLayout;
    radius_ = new QSlider(Qt::Horizontal, sculptBox);
    radius_->setRange(1, 16);
    radius_->setValue(4);
    strength_ = new QSlider(Qt::Horizontal, sculptBox);
    strength_->setRange(1, 50); // tenths: 0.1 .. 5.0
    strength_->setValue(10);
    sliders->addRow(tr("Radius"), radius_);
    sliders->addRow(tr("Strength"), strength_);
    sculptLayout->addLayout(sliders);
    connect(radius_, &QSlider::valueChanged, this, [this] { emitBrush(); });
    connect(strength_, &QSlider::valueChanged, this, [this] { emitBrush(); });
    layout->addWidget(sculptBox);

    auto* generateBox = new QGroupBox(tr("Map Generation"), this);
    auto* generateLayout = new QFormLayout(generateBox);
    seed_ = new QSpinBox(generateBox);
    seed_->setRange(0, 1000000);
    seed_->setValue(1337);
    generateLayout->addRow(tr("Seed"), seed_);
    auto* generateButton = new QPushButton(tr("Generate"), generateBox);
    generateLayout->addRow(generateButton);
    connect(generateButton, &QPushButton::clicked, this, [this] {
        emit generateRequested(static_cast<quint64>(seed_->value()));
    });
    layout->addWidget(generateBox);

    layout->addStretch(1);
}

void TerrainPanel::emitBrush() {
    QString operation;
    for (auto* button : brushGroup_->buttons()) {
        if (button->isChecked()) {
            operation = button->property("operation").toString();
            break;
        }
    }
    emit brushChanged(operation, static_cast<float>(radius_->value()),
                      static_cast<float>(strength_->value()) / 10.0f);
}

} // namespace sky::editor
