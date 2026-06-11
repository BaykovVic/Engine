#pragma once

#include <QWidget>

#include "editor_context.hpp"

namespace sky::editor {

/// The Scene view: paints a side-on (X right, Y up) projection of the world
/// with a Unity-like horizon gradient, unit grid and object gizmos. A real
/// render backend replaces the painting; the layout and interaction stay.
class ViewportWidget final : public QWidget {
    Q_OBJECT

public:
    explicit ViewportWidget(EditorContext& context, QWidget* parent = nullptr);

    void setSelected(object::ObjectHandle object) {
        selected_ = object;
        update();
    }

signals:
    void objectPicked(quint64 objectId);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    [[nodiscard]] QPointF worldToScreen(float x, float y) const;
    [[nodiscard]] QRectF objectRect(object::ObjectHandle object) const;

    EditorContext& context_;
    object::ObjectHandle selected_;
    float pixelsPerUnit_ = 42.0f;
};

} // namespace sky::editor
