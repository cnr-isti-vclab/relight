#include "rusinview.h"

#include <QPainter>
#include <QMouseEvent>
#include <Eigen/Geometry>
#include <cmath>
#include <algorithm>

// ---------------------------------------------------------------------------
RusinkiewiczView::RusinkiewiczView(QWidget *parent) : QWidget(parent) {
    setMinimumSize(minimumSizeHint());
}

void RusinkiewiczView::setData(const std::vector<Eigen::Vector3f> &colors,
                                const std::vector<Eigen::Vector3f> &lights) {
    const size_t n = std::min(colors.size(), lights.size());
    m_colors.assign(colors.begin(), colors.begin() + int(n));
    m_lights.assign(lights.begin(), lights.begin() + int(n));
    m_normal = Eigen::Vector3f(0.f, 0.f, 1.f);
    m_hasData = n > 0;
    recomputeAngles();
    update();
}

void RusinkiewiczView::setNormal(const Eigen::Vector3f &n) {
    m_normal = n.normalized();
    if (m_hasData) { recomputeAngles(); update(); }
}

void RusinkiewiczView::clear() {
    m_colors.clear();
    m_lights.clear();
    m_thetaH.clear();
    m_thetaD.clear();
    m_hasData = false;
    update();
}

void RusinkiewiczView::recomputeAngles() {
    const size_t n = m_colors.size();
    m_thetaH.resize(n);
    m_thetaD.resize(n);

    const Eigen::Vector3f nN = m_normal.normalized();
    const Eigen::Vector3f v(0.f, 0.f, 1.f);

    for (size_t i = 0; i < n; ++i) {
        Eigen::Vector3f l = m_lights[i].normalized();
        Eigen::Vector3f h = (l + v).normalized();

        // θ_h: angle between half-vector and surface normal
        m_thetaH[i] = std::acos(std::clamp(h.dot(nN), -1.f, 1.f)) * 180.f / float(M_PI);

        // θ_d: angle between light and half-vector
        m_thetaD[i] = std::acos(std::clamp(l.dot(h), -1.f, 1.f)) * 180.f / float(M_PI);
    }
}

void RusinkiewiczView::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // ---- Layout ------------------------------------------------------------
    const double marginL = 46, marginR = 18, marginT = 18, marginB = 36;
    const QRectF plot(marginL, marginT,
                      width()  - marginL - marginR,
                      height() - marginT - marginB);

    // ---- Background -------------------------------------------------------
    p.fillRect(rect(), QColor(28, 28, 38));
    p.fillRect(plot, QColor(20, 20, 32));

    // ---- Grid / axis lines ------------------------------------------------
    const int xTicks[] = {0, 15, 30, 45, 60, 75, 90};
    const int yTicks[] = {0, 15, 30, 45, 60, 75, 90};

    p.setPen(QPen(QColor(255, 255, 255, 28), 0.7));
    for (int v : xTicks) {
        double x = plot.left() + plot.width() * v / 90.0;
        p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
    }
    for (int v : yTicks) {
        double y = plot.top() + plot.height() * v / 90.0;
        p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
    }

    // ---- Axis labels -------------------------------------------------------
    QFont fnt = p.font();
    fnt.setPointSizeF(8.0);
    p.setFont(fnt);
    p.setPen(QColor(160, 165, 185));

    for (int v : xTicks) {
        double x = plot.left() + plot.width() * v / 90.0;
        p.drawText(QRectF(x - 16, plot.bottom() + 4, 32, 14),
                   Qt::AlignHCenter | Qt::AlignTop, QString::number(v) + "°");
    }
    for (int v : yTicks) {
        double y = plot.top() + plot.height() * v / 90.0;
        p.drawText(QRectF(2, y - 8, marginL - 6, 16),
                   Qt::AlignRight | Qt::AlignVCenter, QString::number(v) + "°");
    }

    // Axis titles
    QFont titleFnt = fnt;
    titleFnt.setPointSizeF(9.0);
    p.setFont(titleFnt);
    p.setPen(QColor(200, 205, 220));
    p.drawText(QRectF(plot.left(), plot.bottom() + 20, plot.width(), 14),
               Qt::AlignHCenter | Qt::AlignTop, "\u03b8\u2095 (deg)");

    p.save();
    p.translate(10, plot.center().y());
    p.rotate(-90);
    p.drawText(QRectF(-60, -8, 120, 16), Qt::AlignHCenter | Qt::AlignVCenter,
               "\u03c6_d (deg)");
    p.restore();

    // ---- Dots --------------------------------------------------------------
    if (!m_hasData || m_colors.empty()) {
        p.setPen(QColor(120, 120, 140));
        fnt.setPointSizeF(11.0);
        p.setFont(fnt);
        p.drawText(plot.toRect(), Qt::AlignCenter, "Click a pixel to populate");
        return;
    }

    // Scale so the brightest sample reaches full intensity
    float maxLum = 0.f;
    for (const auto &col : m_colors)
        maxLum = std::max(maxLum, 0.299f * col.x() + 0.587f * col.y() + 0.114f * col.z());
    if (maxLum < 1e-9f) maxLum = 1.f;

    const double dotR = 5.5;
    const size_t n = m_colors.size();
    for (size_t i = 0; i < n; ++i) {
        double px = plot.left() + plot.width()  * m_thetaH[i] / 90.0;
        double py = plot.top()  + plot.height() * m_thetaD[i] / 90.0;

        float scale = 1.f / maxLum;
        QColor c(std::clamp(int(m_colors[i].x() * scale * 255.f), 0, 255),
                 std::clamp(int(m_colors[i].y() * scale * 255.f), 0, 255),
                 std::clamp(int(m_colors[i].z() * scale * 255.f), 0, 255));

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 80));
        p.drawEllipse(QPointF(px + 1, py + 1), dotR, dotR);

        p.setBrush(c);
        p.setPen(QPen(c.lighter(150), 0.5));
        p.drawEllipse(QPointF(px, py), dotR, dotR);
    }
}

// ---------------------------------------------------------------------------
void RusinkiewiczView::mousePressEvent(QMouseEvent *event) {
    if (!m_hasData || m_thetaH.empty()) return;

    const double marginL = 46, marginR = 18, marginT = 18, marginB = 36;
    const QRectF plot(marginL, marginT,
                      width()  - marginL - marginR,
                      height() - marginT - marginB);

    const QPointF pos(event->pos());
    const double hitR2 = 10.0 * 10.0;

    int best = -1;
    double bestDist2 = hitR2;
    for (int i = 0; i < int(m_thetaH.size()); ++i) {
        double px = plot.left() + plot.width()  * m_thetaH[i] / 90.0;
        double py = plot.top()  + plot.height() * m_thetaD[i] / 90.0;
        double dx = pos.x() - px, dy = pos.y() - py;
        double d2 = dx*dx + dy*dy;
        if (d2 < bestDist2) { bestDist2 = d2; best = i; }
    }

    if (best >= 0)
        emit lightSelected(best);
}
