#include "viewport_widget.hpp"

#include <QKeyEvent>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <cmath>
#include <functional>

namespace sky::editor {
namespace {

constexpr float kArrowLength = 70.0f;
constexpr float kHandleHitWidth = 9.0f;
constexpr float kCenterHalf = 8.0f;
constexpr float kRingRadius = 56.0f;

const QColor kAxisX(0xdb, 0x4d, 0x4d);
const QColor kAxisY(0x6f, 0xc0, 0x5a);
const QColor kAccent(0xf0, 0xd8, 0x5a);
const QColor kSelection(0x4f, 0x93, 0xd8);

core::Quat conjugate(const core::Quat& q) { return {-q.x, -q.y, -q.z, q.w}; }

core::Quat rotationAroundZ(float radians) {
    return {0.0f, 0.0f, std::sin(radians / 2.0f), std::cos(radians / 2.0f)};
}

float snapValue(float value, float step) {
    return std::round(value / step) * step;
}

float distanceToSegment(QPointF p, QPointF a, QPointF b) {
    const QPointF ab = b - a;
    const float lengthSq = static_cast<float>(QPointF::dotProduct(ab, ab));
    if (lengthSq < 1e-6f) {
        return static_cast<float>(QLineF(p, a).length());
    }
    const float t = std::clamp(
        static_cast<float>(QPointF::dotProduct(p - a, ab)) / lengthSq, 0.0f, 1.0f);
    return static_cast<float>(QLineF(p, a + ab * t).length());
}

} // namespace

ViewportWidget::ViewportWidget(EditorContext& context, QWidget* parent)
    : QWidget(parent), context_(context) {
    setMinimumSize(400, 300);
    setObjectName("viewport");
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

void ViewportWidget::setSelected(object::ObjectHandle object) {
    selected_ = object;
    update();
}

void ViewportWidget::setTool(TransformTool tool) {
    if (tool_ == tool) {
        return;
    }
    tool_ = tool;
    emit toolChanged(static_cast<int>(tool));
    update();
}

void ViewportWidget::frameSelected() {
    if (!context_.objects->exists(selected_)) {
        return;
    }
    const auto world = context_.objects->worldTransform(selected_);
    cameraCenter_ = {world.position.x, world.position.y};
    update();
}

QPointF ViewportWidget::worldToScreen(float x, float y) const {
    return {width() / 2.0 + (x - cameraCenter_.x()) * pixelsPerUnit_,
            height() * 0.6 - (y - cameraCenter_.y()) * pixelsPerUnit_};
}

QPointF ViewportWidget::screenToWorld(QPointF screen) const {
    return {cameraCenter_.x() + (screen.x() - width() / 2.0) / pixelsPerUnit_,
            cameraCenter_.y() - (screen.y() - height() * 0.6) / pixelsPerUnit_};
}

QRectF ViewportWidget::objectRect(object::ObjectHandle object) const {
    const auto transform = context_.objects->worldTransform(object);
    const float w = std::max(0.25f, transform.scale.x) * pixelsPerUnit_;
    const float h = std::max(0.25f, transform.scale.y) * pixelsPerUnit_;
    const auto centre = worldToScreen(transform.position.x, transform.position.y);
    return {centre.x() - w / 2.0, centre.y() - h / 2.0, static_cast<qreal>(w),
            static_cast<qreal>(h)};
}

ViewportWidget::Handle ViewportWidget::handleAt(QPointF position) const {
    if (!context_.objects->exists(selected_) || tool_ == TransformTool::Hand) {
        return Handle::None;
    }
    const auto world = context_.objects->worldTransform(selected_);
    const auto centre = worldToScreen(world.position.x, world.position.y);
    const QPointF xTip = centre + QPointF(kArrowLength, 0);
    const QPointF yTip = centre - QPointF(0, kArrowLength);

    if (tool_ == TransformTool::Move || tool_ == TransformTool::Scale) {
        const bool move = tool_ == TransformTool::Move;
        if (QRectF(centre.x() - kCenterHalf, centre.y() - kCenterHalf,
                   kCenterHalf * 2, kCenterHalf * 2)
                .contains(position)) {
            return move ? Handle::MoveFree : Handle::ScaleUniform;
        }
        if (distanceToSegment(position, centre, xTip) < kHandleHitWidth) {
            return move ? Handle::MoveX : Handle::ScaleX;
        }
        if (distanceToSegment(position, centre, yTip) < kHandleHitWidth) {
            return move ? Handle::MoveY : Handle::ScaleY;
        }
    } else if (tool_ == TransformTool::Rotate) {
        const float distance = static_cast<float>(QLineF(position, centre).length());
        if (std::fabs(distance - kRingRadius) < kHandleHitWidth) {
            return Handle::RotateRing;
        }
    }
    return Handle::None;
}

object::ObjectHandle ViewportWidget::pickObject(QPointF position) const {
    object::ObjectHandle picked;
    const std::function<void(object::ObjectHandle)> hitTest =
        [&](object::ObjectHandle object) {
            if (!context_.objects->exists(object)) {
                return;
            }
            if (objectRect(object).contains(position)) {
                picked = object;
            }
            for (const auto child : context_.objects->childrenOf(object)) {
                hitTest(child);
            }
        };
    for (const auto root : context_.rootObjects()) {
        hitTest(root);
    }
    return picked;
}

void ViewportWidget::mousePressEvent(QMouseEvent* event) {
    setFocus();
    if (event->button() == Qt::MiddleButton || event->button() == Qt::RightButton ||
        (tool_ == TransformTool::Hand && event->button() == Qt::LeftButton)) {
        panning_ = true;
        dragStartScreen_ = event->pos();
        panStartCamera_ = cameraCenter_;
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    if (event->button() != Qt::LeftButton) {
        return;
    }

    // Gizmo handles take priority over picking.
    const auto handle = handleAt(event->pos());
    if (handle != Handle::None) {
        activeHandle_ = handle;
        dragStartScreen_ = event->pos();
        dragStartLocal_ = context_.objects->localTransform(selected_);
        dragStartWorldPosition_ =
            context_.objects->worldTransform(selected_).position;
        update();
        return;
    }

    const auto picked = pickObject(event->pos());
    setSelected(picked);
    emit objectPicked(picked.value);
}

void ViewportWidget::mouseMoveEvent(QMouseEvent* event) {
    if (panning_) {
        const QPointF delta = event->pos() - dragStartScreen_;
        cameraCenter_ = panStartCamera_ + QPointF(-delta.x() / pixelsPerUnit_,
                                                  delta.y() / pixelsPerUnit_);
        update();
        return;
    }
    if (activeHandle_ != Handle::None) {
        applyDrag(event->pos(), event->modifiers().testFlag(Qt::ControlModifier));
        return;
    }

    const auto handle = handleAt(event->pos());
    const auto hovered = handle == Handle::None ? pickObject(event->pos())
                                                : object::ObjectHandle::invalid();
    if (handle != hoverHandle_ || hovered != hovered_) {
        hoverHandle_ = handle;
        hovered_ = hovered;
        updateCursor(handle);
        update();
    }
}

void ViewportWidget::mouseReleaseEvent(QMouseEvent*) {
    if (panning_) {
        panning_ = false;
        updateCursor(hoverHandle_);
    }
    if (activeHandle_ != Handle::None) {
        if (context_.objects->exists(selected_)) {
            const auto after = context_.objects->localTransform(selected_);
            if (after != dragStartLocal_) {
                emit transformCommitted(selected_.value, dragStartLocal_, after);
            }
        }
        activeHandle_ = Handle::None;
        update();
    }
}

void ViewportWidget::wheelEvent(QWheelEvent* event) {
    // Zoom towards the cursor: the world point under it stays put.
    const auto anchorWorld = screenToWorld(event->position());
    const float factor = event->angleDelta().y() > 0 ? 1.12f : 1.0f / 1.12f;
    pixelsPerUnit_ = std::clamp(pixelsPerUnit_ * factor, 6.0f, 400.0f);
    const auto anchorAfter = screenToWorld(event->position());
    cameraCenter_ += anchorWorld - anchorAfter;
    update();
}

void ViewportWidget::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Q: setTool(TransformTool::Hand); return;
        case Qt::Key_W: setTool(TransformTool::Move); return;
        case Qt::Key_E: setTool(TransformTool::Rotate); return;
        case Qt::Key_R: setTool(TransformTool::Scale); return;
        case Qt::Key_F: frameSelected(); return;
        case Qt::Key_Delete:
        case Qt::Key_Backspace:
            if (selected_.isValid()) {
                emit deleteRequested(selected_.value);
            }
            return;
        case Qt::Key_D:
            if (event->modifiers().testFlag(Qt::ControlModifier) &&
                selected_.isValid()) {
                emit duplicateRequested(selected_.value);
                return;
            }
            break;
        case Qt::Key_Z:
            if (event->modifiers().testFlag(Qt::ControlModifier)) {
                if (event->modifiers().testFlag(Qt::ShiftModifier)) {
                    emit redoRequested();
                } else {
                    emit undoRequested();
                }
                return;
            }
            break;
        default: break;
    }
    QWidget::keyPressEvent(event);
}

void ViewportWidget::applyDrag(QPointF screenPosition, bool snap) {
    if (!context_.objects->exists(selected_)) {
        activeHandle_ = Handle::None;
        return;
    }
    const QPointF screenDelta = screenPosition - dragStartScreen_;
    const float dx = static_cast<float>(screenDelta.x()) / pixelsPerUnit_;
    const float dy = -static_cast<float>(screenDelta.y()) / pixelsPerUnit_;
    auto transform = dragStartLocal_;

    switch (activeHandle_) {
        case Handle::MoveX:
        case Handle::MoveY:
        case Handle::MoveFree: {
            core::Vec3 worldDelta{activeHandle_ != Handle::MoveY ? dx : 0.0f,
                                  activeHandle_ != Handle::MoveX ? dy : 0.0f, 0.0f};
            auto worldPosition = dragStartWorldPosition_ + worldDelta;
            if (snap) {
                worldPosition = {snapValue(worldPosition.x, 0.5f),
                                 snapValue(worldPosition.y, 0.5f),
                                 snapValue(worldPosition.z, 0.5f)};
            }
            // Convert the world-space goal into the parent's local space.
            const auto parent = context_.objects->parentOf(selected_);
            if (parent.isValid()) {
                const auto parentWorld = context_.objects->worldTransform(parent);
                const core::Vec3 offset{worldPosition.x - parentWorld.position.x,
                                        worldPosition.y - parentWorld.position.y,
                                        worldPosition.z - parentWorld.position.z};
                auto local = core::rotate(conjugate(parentWorld.rotation), offset);
                transform.position = {local.x / parentWorld.scale.x,
                                      local.y / parentWorld.scale.y,
                                      local.z / parentWorld.scale.z};
            } else {
                transform.position = worldPosition;
            }
            break;
        }
        case Handle::ScaleX:
        case Handle::ScaleY:
        case Handle::ScaleUniform: {
            const float uniform = 1.0f + (dx + dy) * 0.5f;
            float factorX = activeHandle_ == Handle::ScaleX ? 1.0f + dx : 1.0f;
            float factorY = activeHandle_ == Handle::ScaleY ? 1.0f + dy : 1.0f;
            if (activeHandle_ == Handle::ScaleUniform) {
                factorX = factorY = uniform;
            }
            transform.scale.x = std::max(0.05f, dragStartLocal_.scale.x * factorX);
            transform.scale.y = std::max(0.05f, dragStartLocal_.scale.y * factorY);
            if (snap) {
                transform.scale.x = std::max(0.25f, snapValue(transform.scale.x, 0.25f));
                transform.scale.y = std::max(0.25f, snapValue(transform.scale.y, 0.25f));
            }
            break;
        }
        case Handle::RotateRing: {
            const auto world = context_.objects->worldTransform(selected_);
            const auto centre = worldToScreen(world.position.x, world.position.y);
            const float startAngle =
                std::atan2(-(dragStartScreen_.y() - centre.y()),
                           dragStartScreen_.x() - centre.x());
            const float currentAngle = std::atan2(
                -(screenPosition.y() - centre.y()), screenPosition.x() - centre.x());
            float delta = currentAngle - startAngle;
            if (snap) {
                delta = snapValue(delta, static_cast<float>(M_PI) / 12.0f); // 15°
            }
            transform.rotation = rotationAroundZ(delta) * dragStartLocal_.rotation;
            break;
        }
        case Handle::None:
            return;
    }

    context_.objects->setLocalTransform(selected_, transform);
    emit transformEdited();
    update();
}

void ViewportWidget::updateCursor(Handle handle) {
    if (tool_ == TransformTool::Hand) {
        setCursor(Qt::OpenHandCursor);
        return;
    }
    switch (handle) {
        case Handle::MoveX:
        case Handle::ScaleX: setCursor(Qt::SizeHorCursor); break;
        case Handle::MoveY:
        case Handle::ScaleY: setCursor(Qt::SizeVerCursor); break;
        case Handle::MoveFree:
        case Handle::ScaleUniform: setCursor(Qt::SizeAllCursor); break;
        case Handle::RotateRing: setCursor(Qt::CrossCursor); break;
        case Handle::None: setCursor(Qt::ArrowCursor); break;
    }
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

            // Rotation around Z is visualized by rotating the painted box.
            const auto rotation = context_.objects->worldTransform(object).rotation;
            const float angle =
                std::atan2(2.0f * (rotation.w * rotation.z + rotation.x * rotation.y),
                           1.0f - 2.0f * (rotation.y * rotation.y +
                                          rotation.z * rotation.z));
            painter.save();
            painter.translate(box.center());
            painter.rotate(-angle * 180.0 / M_PI);
            painter.drawRect(QRectF(-box.width() / 2, -box.height() / 2, box.width(),
                                    box.height()));
            painter.restore();

            if (object == hovered_ && object != selected_) {
                painter.setBrush(Qt::NoBrush);
                painter.setPen(QPen(QColor(255, 255, 255, 90), 1.5));
                painter.drawRect(box.adjusted(-2, -2, 2, 2));
            }
            if (object == selected_) {
                painter.setBrush(Qt::NoBrush);
                painter.setPen(QPen(kSelection, 2));
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

    drawGizmo(painter);

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

void ViewportWidget::drawGizmo(QPainter& painter) {
    if (!context_.objects->exists(selected_) || tool_ == TransformTool::Hand) {
        return;
    }
    const auto world = context_.objects->worldTransform(selected_);
    const auto centre = worldToScreen(world.position.x, world.position.y);
    const QPointF xTip = centre + QPointF(kArrowLength, 0);
    const QPointF yTip = centre - QPointF(0, kArrowLength);

    const auto axisColor = [&](Handle handle, const QColor& base) {
        return (activeHandle_ == handle || hoverHandle_ == handle) ? kAccent : base;
    };

    if (tool_ == TransformTool::Move || tool_ == TransformTool::Scale) {
        const bool move = tool_ == TransformTool::Move;
        const auto xHandle = move ? Handle::MoveX : Handle::ScaleX;
        const auto yHandle = move ? Handle::MoveY : Handle::ScaleY;
        const auto centreHandle = move ? Handle::MoveFree : Handle::ScaleUniform;

        painter.setPen(QPen(axisColor(xHandle, kAxisX), 2.5));
        painter.drawLine(centre, xTip);
        painter.setPen(QPen(axisColor(yHandle, kAxisY), 2.5));
        painter.drawLine(centre, yTip);

        if (move) {
            // Arrowheads.
            painter.setPen(Qt::NoPen);
            painter.setBrush(axisColor(xHandle, kAxisX));
            painter.drawPolygon(QPolygonF()
                                << xTip << xTip + QPointF(-10, -5)
                                << xTip + QPointF(-10, 5));
            painter.setBrush(axisColor(yHandle, kAxisY));
            painter.drawPolygon(QPolygonF()
                                << yTip << yTip + QPointF(-5, 10)
                                << yTip + QPointF(5, 10));
        } else {
            // Scale boxes at the tips.
            painter.setPen(Qt::NoPen);
            painter.setBrush(axisColor(xHandle, kAxisX));
            painter.drawRect(QRectF(xTip.x() - 5, xTip.y() - 5, 10, 10));
            painter.setBrush(axisColor(yHandle, kAxisY));
            painter.drawRect(QRectF(yTip.x() - 5, yTip.y() - 5, 10, 10));
        }

        painter.setBrush(axisColor(centreHandle, QColor(0xd8, 0xd8, 0xd8)));
        painter.setPen(QPen(QColor(0, 0, 0, 120), 1));
        painter.drawRect(QRectF(centre.x() - kCenterHalf, centre.y() - kCenterHalf,
                                kCenterHalf * 2, kCenterHalf * 2));
    } else if (tool_ == TransformTool::Rotate) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(axisColor(Handle::RotateRing, QColor(0x6f, 0xa8, 0xdc)),
                            2.5));
        painter.drawEllipse(centre, kRingRadius, kRingRadius);
        // Angle marker.
        const auto rotation = world.rotation;
        const float angle =
            std::atan2(2.0f * (rotation.w * rotation.z + rotation.x * rotation.y),
                       1.0f - 2.0f * (rotation.y * rotation.y +
                                      rotation.z * rotation.z));
        painter.drawLine(centre,
                         centre + QPointF(std::cos(angle) * kRingRadius,
                                          -std::sin(angle) * kRingRadius));
    }
}

} // namespace sky::editor
