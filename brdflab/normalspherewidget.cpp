#include "normalspherewidget.h"

#include <QPainter>
#include <QMouseEvent>
#include <cmath>

NormalSphereWidget::NormalSphereWidget(QWidget *parent) : QWidget(parent) {
    setMinimumSize(minimumSizeHint());
    setCursor(Qt::ArrowCursor);
}

NormalSphereWidget::Layout NormalSphereWidget::layout() const {
    const double margin = 10.0;
    const double textH  = 44.0;
    const double side   = std::min(width()  - 2 * margin,
                                   height() - 2 * margin - textH);
    return { width() * 0.5, margin + side * 0.5, side * 0.5 };
}

Eigen::Vector3f NormalSphereWidget::normalFromPoint(double px, double py) const {
    auto l = layout();
    double nx =  (px - l.cx) / l.r;
    double ny = -(py - l.cy) / l.r;  // screen y flipped
    double len2 = nx * nx + ny * ny;
    // Clamp to the unit disc; z is always >= 0 (upper hemisphere)
    if (len2 > 1.0) { double s = 1.0 / std::sqrt(len2); nx *= s; ny *= s; len2 = 1.0; }
    double nz = std::sqrt(std::max(0.0, 1.0 - len2));
    return Eigen::Vector3f(float(nx), float(ny), float(nz)).normalized();
}

void NormalSphereWidget::setNormal(const Eigen::Vector3f &n) {
    normal    = n.normalized();
    hasNormal = true;
    setCursor(Qt::OpenHandCursor);
    update();
}

void NormalSphereWidget::clear() {
    hasNormal = false;
    dragging  = false;
    setCursor(Qt::ArrowCursor);
    update();
}

void NormalSphereWidget::mousePressEvent(QMouseEvent *event) {
    if (!hasNormal) return;
    dragging = true;
    setCursor(Qt::ClosedHandCursor);
    normal = normalFromPoint(event->pos().x(), event->pos().y());
    update();
}

void NormalSphereWidget::mouseMoveEvent(QMouseEvent *event) {
    if (!dragging) return;
    normal = normalFromPoint(event->pos().x(), event->pos().y());
    update();
    emit normalChanged(normal);
}

void NormalSphereWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (!dragging) return;
    dragging = false;
    setCursor(Qt::OpenHandCursor);
    normal = normalFromPoint(event->pos().x(), event->pos().y());
    update();
    emit normalChanged(normal);
}

void NormalSphereWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF rect  = this->rect();
    const double margin = 10.0;
    const double textH  = 44.0;   // reserved at bottom for coordinate text

    const double side = std::min(rect.width() - 2 * margin,
                                 rect.height() - 2 * margin - textH);
    const double r  = side * 0.5;
    const double cx = rect.width()  * 0.5;
    const double cy = margin + r;

    // ---- Background disc ----
    p.setBrush(QColor(40, 50, 75));
    p.setPen(QPen(QColor(180, 190, 210), 1.5));
    p.drawEllipse(QPointF(cx, cy), r, r);

    // ---- Concentric elevation rings (every 30°) ----
    // Each ring at elevation angle e projects to radius r * cos(e)
    p.setPen(QPen(QColor(255, 255, 255, 50), 0.8));
    for (int e = 30; e < 90; e += 30) {
        double ringR = r * std::cos(e * M_PI / 180.0);
        p.drawEllipse(QPointF(cx, cy), ringR, ringR);
    }

    // ---- Cross-hair axes ----
    p.setPen(QPen(QColor(255, 255, 255, 60), 0.8));
    p.drawLine(QPointF(cx - r, cy), QPointF(cx + r, cy));
    p.drawLine(QPointF(cx, cy - r), QPointF(cx, cy + r));

    // ---- Axis labels ----
    p.setPen(QColor(160, 170, 190));
    QFont fnt = p.font();
    fnt.setPointSizeF(8.0);
    p.setFont(fnt);
    p.drawText(QRectF(cx + r + 2, cy - 8, 20, 16),  Qt::AlignLeft   | Qt::AlignVCenter, "X");
    p.drawText(QRectF(cx - r - 20, cy - 8, 18, 16), Qt::AlignRight  | Qt::AlignVCenter, "-X");
    p.drawText(QRectF(cx - 10, cy - r - 14, 20, 14), Qt::AlignHCenter | Qt::AlignTop,  "Y");
    p.drawText(QRectF(cx - 10, cy + r + 2,  20, 14), Qt::AlignHCenter | Qt::AlignTop,  "-Y");

    // ---- Normal dot ----
    if (hasNormal) {
        // Top-down (XY) projection: x→right, y→up
        double dx =  double(normal.x()) * r;
        double dy = -double(normal.y()) * r;  // screen y is flipped

        // Shadow
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 100));
        p.drawEllipse(QPointF(cx + dx + 1.5, cy + dy + 1.5), 6, 6);
        // Dot (red-orange)
        p.setBrush(QColor(255, 80, 60));
        p.setPen(QPen(Qt::white, 1.2));
        p.drawEllipse(QPointF(cx + dx, cy + dy), 6, 6);

        // ---- Coordinate text ----
        const QString coords = QString("n = (%1, %2, %3)")
                                   .arg(double(normal.x()), 0, 'f', 3)
                                   .arg(double(normal.y()), 0, 'f', 3)
                                   .arg(double(normal.z()), 0, 'f', 3);
        p.setPen(QColor(220, 220, 220));
        QFont cf = p.font();
        cf.setPointSizeF(10.0);
        p.setFont(cf);
        p.drawText(QRectF(margin, cy + r + 8, rect.width() - 2 * margin, textH),
                   Qt::AlignHCenter | Qt::AlignTop, coords);
    } else {
        p.setPen(QColor(140, 140, 140));
        QFont cf = p.font();
        cf.setPointSizeF(9.5);
        p.setFont(cf);
        p.drawText(QRectF(margin, cy + r + 8, rect.width() - 2 * margin, textH),
                   Qt::AlignHCenter | Qt::AlignTop, "Click a pixel to compute normal");
    }
}

