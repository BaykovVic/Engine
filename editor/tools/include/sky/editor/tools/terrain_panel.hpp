#pragma once

#include <QWidget>

class QPushButton;
class QSlider;
class QSpinBox;
class QButtonGroup;

namespace sky::editor {

/// Unity-like terrain tools: sculpting brushes (raise/lower/smooth) with
/// radius/strength, plus procedural map generation by seed. Pure UI — the
/// shell routes its signals into the engine.
class TerrainPanel final : public QWidget {
    Q_OBJECT

public:
    explicit TerrainPanel(QWidget* parent = nullptr);

signals:
    /// operation is empty when sculpting is switched off.
    void brushChanged(QString operation, float radius, float strength);
    void generateRequested(quint64 seed);

private:
    void emitBrush();

    QButtonGroup* brushGroup_ = nullptr;
    QSlider* radius_ = nullptr;
    QSlider* strength_ = nullptr;
    QSpinBox* seed_ = nullptr;
};

} // namespace sky::editor
