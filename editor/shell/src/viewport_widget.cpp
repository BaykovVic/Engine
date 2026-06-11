#include "viewport_widget.hpp"

#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <cmath>
#include <functional>

namespace sky::editor {

ViewportWidget::ViewportWidget(EditorContext& context, QWidget* parent)
    : QWidget(parent), context_(context) {
    setMinimumSize(400, 300);
    setObjectName("viewport");
}

QPointF ViewportWidget::worldToScreen(float x, float y) const {
    // World origin sits at the horizontal centre, ground line in the lower
    // third of the view.
    const float originX = width() / 2.0f;
    const float originY = height() * 0.72f;
    return {originX + x * pixelsPerUnit_, originY - y * pixelsPerUnit_};
}

QRectF ViewportWidget::objectRect(object::ObjectHandle object) const {
    const auto transform = context_.objects->worldTransform(object);
    const float w = std::max(0.25f, transform.scale.x) * pixelsPerUnit_;
    const float h = std::max(0.25f, transform.scale.y) * pixelsPerUnit_;
    const auto centre = worldToScreen(transform.position.x, transform.position.y);
    return {centre.x() - w / 2.0, centre.y() - h / 2.0, static_cast<qreal>(w),
            static_cast<qreal>(h)};
}

void ViewportWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Sky-to-ground gradient backdrop.
    QLinearGradient sky(0, 0, 0, height());
    sky.setColorAt(0.0, QColor(0x37, 0x47, 0x5a));
    sky.setColorAt(0.7, QColor(0x24, 0x2c, 0x35));
    sky.setColorAt(1.0, QColor(0x1b, 0x20, 0x26));
    painter.fillRect(rect(), sky);

    // Unit grid with a highlighted ground axis.
    painter.setPen(QPen(QColor(255, 255, 255, 16), 1));
    const auto origin = worldToScreen(0.0f, 0.0f);
    for (float x = std::fmod(origin.x(), pixelsPerUnit_); x < width();
         x += pixelsPerUnit_) {
        painter.drawLine(QPointF(x, 0), QPointF(x, height()));
    }
    for (float y = std::fmod(origin.y(), pixelsPerUnit_); y < height();
         y += pixelsPerUnit_) {
        painter.drawLine(QPointF(0, y), QPointF(width(), y));
    }
    painter.setPen(QPen(QColor(120, 170, 220, 70), 2));
    painter.drawLine(QPointF(0, origin.y()), QPointF(width(), origin.y()));

    // Objects, recursively, as Unity-like gizmo boxes.
    const std::function<void(object::ObjectHandle)> drawObject =
        [&](object::ObjectHandle object) {
            if (!context_.objects->exists(object)) {
                return;
            }
            const auto name =
                QString::fromStdString(context_.objects->nameOf(object));
            const auto box = objectRect(object);

            QColor fill(0xcb, 0x8d, 0x46, 200); // crate orange
            if (name.contains("Ground")) {
                fill = QColor(0x5a, 0x77, 0x4f, 220);
            } else if (name.contains("Camera")) {
                fill = QColor(0x7d, 0x8a, 0x99, 160);
            } else if (name.contains("Light")) {
                fill = QColor(0xe8, 0xd9, 0x6b, 160);
            }
            painter.setBrush(fill);
            painter.setPen(QPen(fill.darker(140), 1.5));
            painter.drawRect(box);

            if (object == selected_) {
                painter.setBrush(Qt::NoBrush);
                painter.setPen(QPen(QColor(0x4f, 0x93, 0xd8), 2));
                painter.drawRect(box.adjusted(-3, -3, 3, 3));
            }

            painter.setPen(QColor(0xd8, 0xd8, 0xd8));
            painter.drawText(QPointF(box.center().x() - name.size() * 3.2,
                                     box.top() - 6),
                             name);

            for (const auto child : context_.objects->childrenOf(object)) {
                drawObject(child);
            }
        };
    for (const auto root : context_.rootObjects()) {
        drawObject(root);
    }

    // Play state badge, Unity-style centred at the top.
    const auto state = context_.playMode->state();
    if (state != PlayModeState::Editing) {
        const auto text = state == PlayModeState::Playing ? tr("▶ Playing")
                                                          : tr("⏸ Paused");
        painter.setPen(QColor(0xff, 0xff, 0xff, 200));
        painter.fillRect(QRectF(width() / 2.0 - 50, 8, 100, 22),
                         QColor(0, 0, 0, 110));
        painter.drawText(QRectF(width() / 2.0 - 50, 8, 100, 22), Qt::AlignCenter,
                         text);
    }
}

void ViewportWidget::mousePressEvent(QMouseEvent* event) {
    // Pick the topmost object whose gizmo contains the click.
    object::ObjectHandle picked;
    const std::function<void(object::ObjectHandle)> hitTest =
        [&](object::ObjectHandle object) {
            if (!context_.objects->exists(object)) {
                return;
            }
            if (objectRect(object).contains(event->pos())) {
                picked = object;
            }
            for (const auto child : context_.objects->childrenOf(object)) {
                hitTest(child);
            }
        };
    for (const auto root : context_.rootObjects()) {
        hitTest(root);
    }
    setSelected(picked);
    emit objectPicked(picked.value);
}

} // namespace sky::editor
