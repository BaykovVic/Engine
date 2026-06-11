#pragma once

#include <QWidget>

#include "sky/rendering/material.hpp"

class QLineEdit;
class QListWidget;
class QPushButton;
class QSlider;

namespace sky::editor {

/// Unity-like material editor: the material library's contents with live
/// editing of colour, surface parameters and the albedo texture path.
class MaterialPanel final : public QWidget {
    Q_OBJECT

public:
    explicit MaterialPanel(rendering::IMaterialLibrary& materials,
                           QWidget* parent = nullptr);

    void refresh();

signals:
    /// Any material changed; viewports re-render with the new parameters.
    void materialsChanged();

private:
    void showSelected();
    void applyEdits();
    void pickColor(bool emissive);
    void createMaterial();
    [[nodiscard]] QString selectedName() const;

    rendering::IMaterialLibrary& materials_;
    bool updating_ = false;
    int newMaterialCounter_ = 0;

    QListWidget* list_ = nullptr;
    QLineEdit* nameEdit_ = nullptr;
    QPushButton* colorButton_ = nullptr;
    QPushButton* emissiveButton_ = nullptr;
    QSlider* roughness_ = nullptr;
    QSlider* metallic_ = nullptr;
    QLineEdit* textureEdit_ = nullptr;
};

} // namespace sky::editor
