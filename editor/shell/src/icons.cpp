#include "icons.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QtMath>

#include <cmath>

namespace sky::editor {
namespace {

// Icons are authored in a 0..100 logical square and scaled to the pixmap.
constexpr int kRender = 64;

void strokeArrowHead(QPainter& p, QPointF tip, QPointF dir, double size) {
    // Two short strokes forming a chevron arrowhead pointing along `dir`.
    const double len = std::hypot(dir.x(), dir.y());
    const QPointF f{dir.x() / len, dir.y() / len};
    const QPointF perp{-f.y(), f.x()};
    const QPointF base = tip - f * size;
    p.drawLine(tip, base + perp * size * 0.7);
    p.drawLine(tip, base - perp * size * 0.7);
}

void drawGlyph(QPainter& p, const QString& name, const QColor& color) {
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 7.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    if (name == "move") {
        p.drawLine(QPointF(50, 16), QPointF(50, 84));
        p.drawLine(QPointF(16, 50), QPointF(84, 50));
        strokeArrowHead(p, {50, 16}, {0, -1}, 13);
        strokeArrowHead(p, {50, 84}, {0, 1}, 13);
        strokeArrowHead(p, {16, 50}, {-1, 0}, 13);
        strokeArrowHead(p, {84, 50}, {1, 0}, 13);
    } else if (name == "rotate") {
        QPainterPath arc;
        const QRectF box(20, 20, 60, 60);
        arc.arcMoveTo(box, 60);
        arc.arcTo(box, 60, 250);
        p.drawPath(arc);
        // Arrowhead at the open end (start of the arc, angle 60 deg).
        const QPointF tip(50 + 30 * std::cos(-60 * M_PI / 180.0),
                          50 + 30 * std::sin(-60 * M_PI / 180.0));
        strokeArrowHead(p, tip, {1.0, -0.3}, 13);
    } else if (name == "scale") {
        p.drawLine(QPointF(30, 70), QPointF(70, 30));
        strokeArrowHead(p, {30, 70}, {-1, 1}, 12);
        strokeArrowHead(p, {70, 30}, {1, -1}, 12);
        p.setBrush(color);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRectF(20, 60, 18, 18), 4, 4);
        p.drawRoundedRect(QRectF(62, 22, 18, 18), 4, 4);
    } else if (name == "hand") {
        // Filled stylised hand: palm plus four fingers and a thumb.
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        QPainterPath hand;
        hand.addRoundedRect(QRectF(34, 44, 36, 36), 12, 12);   // palm
        hand.addRoundedRect(QRectF(37, 22, 8, 30), 4, 4);      // finger 1
        hand.addRoundedRect(QRectF(47, 16, 8, 36), 4, 4);      // finger 2
        hand.addRoundedRect(QRectF(57, 18, 8, 34), 4, 4);      // finger 3
        hand.addRoundedRect(QRectF(66, 26, 8, 26), 4, 4);      // finger 4
        QPainterPath thumb;
        thumb.addRoundedRect(QRectF(22, 44, 24, 9), 4, 4);     // thumb
        QTransform t;
        t.translate(34, 48);
        t.rotate(-35);
        t.translate(-34, -48);
        hand.addPath(t.map(thumb));
        p.drawPath(hand.simplified());
    } else if (name == "play") {
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        QPainterPath tri;
        tri.moveTo(36, 26);
        tri.lineTo(36, 74);
        tri.lineTo(76, 50);
        tri.closeSubpath();
        p.drawPath(tri);
    } else if (name == "pause") {
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawRoundedRect(QRectF(34, 28, 11, 44), 3, 3);
        p.drawRoundedRect(QRectF(55, 28, 11, 44), 3, 3);
    } else if (name == "stop") {
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawRoundedRect(QRectF(32, 32, 36, 36), 6, 6);
    } else if (name == "chevron") {
        p.drawLine(QPointF(30, 42), QPointF(50, 60));
        p.drawLine(QPointF(50, 60), QPointF(70, 42));
    }
}

QPixmap renderGlyph(const QString& name, const QColor& color) {
    QPixmap pixmap(kRender, kRender);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.scale(kRender / 100.0, kRender / 100.0);
    drawGlyph(painter, name, color);
    return pixmap;
}

/// Some icons read better with their own accent so transport reads at a
/// glance: green play, soft-red stop.
QColor accentFor(const QString& name, bool on) {
    if (name == "play") {
        return on ? QColor(0x6f, 0xd4, 0x83) : QColor(0x57, 0xc7, 0x6e);
    }
    if (name == "stop") {
        return QColor(0xe0, 0x6c, 0x68);
    }
    if (name == "pause") {
        return QColor(0xe6, 0xc1, 0x6a);
    }
    return on ? QColor(0xff, 0xff, 0xff) : QColor(0xb6, 0xbc, 0xc6);
}

} // namespace

QIcon toolbarIcon(const QString& name) {
    QIcon icon;
    icon.addPixmap(renderGlyph(name, accentFor(name, false)), QIcon::Normal,
                   QIcon::Off);
    icon.addPixmap(renderGlyph(name, accentFor(name, true)), QIcon::Normal,
                   QIcon::On);
    icon.addPixmap(renderGlyph(name, accentFor(name, true)), QIcon::Active,
                   QIcon::Off);
    return icon;
}

} // namespace sky::editor
