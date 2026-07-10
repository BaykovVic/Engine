#pragma once

#include <QWidget>

#include "editor_context.hpp"

namespace sky::editor {

enum class TransformTool {
    Hand,   // Q: drag to pan
    Move,   // W
    Rotate, // E
    Scale,  // R
};

/// The Scene view: a 2D (X right, Y up) projection of the world with a
/// Unity-like camera (pan/zoom), pick-to-select and draggable
/// move/rotate/scale gizmos. Ctrl snaps while dragging; F frames the
/// selection. A render backend replaces the painting; interaction stays.
class ViewportWidget final : public QWidget {
    Q_OBJECT

public:
    explicit ViewportWidget(EditorContext& context, QWidget* parent = nullptr);

    void setSelected(object::ObjectHandle object);
    void setTool(TransformTool tool);
    [[nodiscard]] TransformTool tool() const { return tool_; }
    void frameSelected();

signals:
    void objectPicked(quint64 objectId);
    void toolChanged(int tool);
    void transformEdited();
    /// A finished gizmo drag: one undoable step from `before` to `after`.
    void transformCommitted(quint64 objectId, sky::core::Transform before,
                            sky::core::Transform after);
    void deleteRequested(quint64 objectId);
    void duplicateRequested(quint64 objectId);
    void undoRequested();
    void redoRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    enum class Handle {
        None,
        MoveX,
        MoveY,
        MoveFree,
        ScaleX,
        ScaleY,
        ScaleUniform,
        RotateRing,
    };

    [[nodiscard]] QPointF worldToScreen(float x, float y) const;
    [[nodiscard]] QPointF screenToWorld(QPointF screen) const;
    [[nodiscard]] QRectF objectRect(object::ObjectHandle object) const;
    [[nodiscard]] Handle handleAt(QPointF position) const;
    [[nodiscard]] object::ObjectHandle pickObject(QPointF position) const;
    void applyDrag(QPointF screenPosition, bool snap);
    void drawGizmo(QPainter& painter);
    void updateCursor(Handle handle);

    EditorContext& context_;
    object::ObjectHandle selected_;
    object::ObjectHandle hovered_;
    TransformTool tool_ = TransformTool::Move;

    // Camera: the world point at the view anchor plus the zoom level.
    QPointF cameraCenter_{0.0, 2.0};
    float pixelsPerUnit_ = 42.0f;

    // Active interaction.
    Handle activeHandle_ = Handle::None;
    Handle hoverHandle_ = Handle::None;
    bool panning_ = false;
    QPointF dragStartScreen_;
    QPointF panStartCamera_;
    core::Transform dragStartLocal_;
    core::Vec3 dragStartWorldPosition_;
};

} // namespace sky::editor
