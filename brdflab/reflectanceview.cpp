#include "reflectanceview.h"

#include <QPainter>
#include <cmath>
#include <algorithm>

ReflectanceView::ReflectanceView(QWidget *parent) : QWidget(parent) {
    setMinimumSize(minimumSizeHint());
}

void ReflectanceView::setData(const std::vector<Eigen::Vector3f> &colors,
                               const std::vector<Eigen::Vector3f> &lights) {
    const size_t n = std::min(colors.size(), lights.size());
    m_colors.assign(colors.begin(),  colors.begin()  + int(n));
    m_lights.assign(lights.begin(),  lights.begin()  + int(n));
    m_hasData = n > 0;
    update();
}

void ReflectanceView::clear() {
    m_colors.clear();
    m_lights.clear();
    m_hasData = false;
    update();
}

ReflectanceView::Layout ReflectanceView::layout() const {
    const double margin = 10.0;
    const double textH  = 20.0;
    const double side   = std::min(width()  - 2 * margin,
                                   height() - 2 * margin - textH);
    return { width() * 0.5, margin + side * 0.5, side * 0.5 };
}

void ReflectanceView::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // ---- Background -------------------------------------------------------
    p.fillRect(rect(), QColor(28, 28, 38));

    auto l = layout();
    const double cx = l.cx, cy = l.cy, r = l.r;

    // ---- Hemisphere disc --------------------------------------------------
    p.setBrush(QColor(20, 20, 32));
    p.setPen(QPen(QColor(180, 190, 210), 1.5));
    p.drawEllipse(QPointF(cx, cy), r, r);

    // ---- Elevation rings (30° and 60°) ------------------------------------
    p.setPen(QPen(QColor(255, 255, 255, 40), 0.8));
    for (int elev = 30; elev < 90; elev += 30) {
        double ringR = r * std::cos(elev * M_PI / 180.0);
        p.drawEllipse(QPointF(cx, cy), ringR, ringR);
    }

    // ---- Crosshair axes ---------------------------------------------------
    p.setPen(QPen(QColor(255, 255, 255, 40), 0.8));
    p.drawLine(QPointF(cx - r, cy), QPointF(cx + r, cy));
    p.drawLine(QPointF(cx, cy - r), QPointF(cx, cy + r));

    // ---- Axis labels -------------------------------------------------------
    QFont fnt = p.font();
    fnt.setPointSizeF(8.0);
    p.setFont(fnt);
    p.setPen(QColor(140, 150, 170));
    const double lbl = 6.0;
    p.drawText(QRectF(cx + r + lbl, cy - 8, 20, 16),  Qt::AlignLeft   | Qt::AlignVCenter, "X");
    p.drawText(QRectF(cx - r - 22,  cy - 8, 20, 16),  Qt::AlignRight  | Qt::AlignVCenter, "-X");
    p.drawText(QRectF(cx - 10,      cy - r - 16, 20, 14), Qt::AlignHCenter | Qt::AlignTop, "Y");
    p.drawText(QRectF(cx - 10,      cy + r + 2,  20, 14), Qt::AlignHCenter | Qt::AlignTop, "-Y");

    if (!m_hasData)
        return;

    // ---- Dots --------------------------------------------------------------
    // Find max brightness for normalising dot opacity / brightness scaling
    float maxLum = 0.f;
    for (const auto &c : m_colors) {
        float lum = 0.299f * c.x() + 0.587f * c.y() + 0.114f * c.z();
        maxLum = std::max(maxLum, lum);
    }
    if (maxLum < 1e-6f) maxLum = 1.f;

    const double dotR = 5.0;

    for (size_t i = 0; i < m_lights.size(); ++i) {
        const Eigen::Vector3f &li = m_lights[i];
        // Map light (x, y) → screen: screen-y is flipped
        double sx = cx + li.x() * r;
        double sy = cy - li.y() * r;

        // Skip lights below horizon (lz < 0) — they shouldn't be lit anyway
        if (li.z() < 0.f) continue;

        const Eigen::Vector3f &col = m_colors[i];
        // Scale up so the brightest light reaches full intensity
        float scale = 1.f / maxLum;
        int ri = std::clamp(int(col.x() * scale * 255.f), 0, 255);
        int gi = std::clamp(int(col.y() * scale * 255.f), 0, 255);
        int bi = std::clamp(int(col.z() * scale * 255.f), 0, 255);

        QColor dotColor(ri, gi, bi);

        // Shadow
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 70));
        p.drawEllipse(QPointF(sx + 1.2, sy + 1.2), dotR, dotR);

        // Dot
        p.setBrush(dotColor);
        p.setPen(QPen(dotColor.lighter(140), 0.6));
        p.drawEllipse(QPointF(sx, sy), dotR, dotR);
    }

    // ---- Title label -------------------------------------------------------
    p.setPen(QColor(160, 165, 185));
    QFont titleFnt = p.font();
    titleFnt.setPointSizeF(8.5);
    p.setFont(titleFnt);
    p.drawText(QRectF(0, height() - 18, width(), 16),
               Qt::AlignHCenter | Qt::AlignTop,
               "Light direction (x, y)");
}
