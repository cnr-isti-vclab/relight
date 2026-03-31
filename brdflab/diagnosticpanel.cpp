#include "diagnosticpanel.h"

#include <QTabWidget>
#include <QVBoxLayout>
#include <QBarSet>
#include <QBarSeries>
#include <QValueAxis>
#include <QScatterSeries>

#include <algorithm>
#include <cmath>
#include <functional>
#include <Eigen/Geometry>

DiagnosticPanel::DiagnosticPanel(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *tabs = new QTabWidget(this);

    chartByLight = new QChart();
    chartByLight->setTitle("Luminosity by light index");
    chartByLight->legend()->setVisible(false);
    tabs->addTab(makeChartView(chartByLight, this), "By light");

    chartDiffuse = new QChart();
    chartDiffuse->setTitle("Luminosity vs angle(L, N)  \u2014 diffuse");
    chartDiffuse->legend()->setVisible(false);
    tabs->addTab(makeChartView(chartDiffuse, this), "L\u00b7N");

    chartSpecular = new QChart();
    chartSpecular->setTitle("Luminosity vs angle(H, N)  \u2014 specular");
    chartSpecular->legend()->setVisible(false);
    tabs->addTab(makeChartView(chartSpecular, this), "H\u00b7N");

    chartPhiD = new QChart();
    chartPhiD->setTitle("Luminosity vs \u03c6_d (azimuthal angle)");
    chartPhiD->legend()->setVisible(false);
    tabs->addTab(makeChartView(chartPhiD, this), "\u03c6_d");

    layout->addWidget(tabs);
    setLayout(layout);
}

QChartView *DiagnosticPanel::makeChartView(QChart *chart, QWidget *parent) {
    auto *cv = new QChartView(chart, parent);
    cv->setRenderHint(QPainter::Antialiasing);
    return cv;
}

void DiagnosticPanel::clearChart(QChart *chart) {
    chart->removeAllSeries();
    const auto axes = chart->axes();
    for (auto *axis : axes) {
        chart->removeAxis(axis);
        delete axis;
    }
}

void DiagnosticPanel::update(const std::vector<float> &luminosities,
                              const std::vector<Eigen::Vector3f> &lights) {
    if (luminosities.empty() || lights.empty()) return;
    const size_t n = std::min(luminosities.size(), lights.size());
    m_luminosities.assign(luminosities.begin(), luminosities.begin() + int(n));
    m_lights.assign(lights.begin(), lights.begin() + int(n));
    const float maxLum = *std::max_element(m_luminosities.begin(), m_luminosities.end());

    // ---- 1. By light index (bar chart) ------------------------------------
    {
        clearChart(chartByLight);
        auto *barSet = new QBarSet("Luminosity");
        for (size_t i = 0; i < n; ++i)
            *barSet << m_luminosities[i];
        auto *series = new QBarSeries();
        series->append(barSet);
        connect(series, &QBarSeries::clicked, this, [this](int idx, QBarSet*) {
            emit lightSelected(idx);
        });
        chartByLight->addSeries(series);

        auto *axisX = new QValueAxis();
        axisX->setRange(0, double(n));
        axisX->setTitleText("Light index");
        chartByLight->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);

        auto *axisY = new QValueAxis();
        axisY->setRange(0, double(maxLum) * 1.05);
        axisY->setTitleText("Luminosity");
        chartByLight->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
    }

    // Clear normal-dependent charts — will be filled by setNormal()
    clearChart(chartDiffuse);
    clearChart(chartSpecular);

    // ---- 4. \u03c6_d azimuthal -----------------------------------------------
    {
        const Eigen::Vector3f v(0.f, 0.f, 1.f);
        clearChart(chartPhiD);
        auto *series = new QScatterSeries();
        series->setMarkerSize(5.0);
        series->setMarkerShape(QScatterSeries::MarkerShapeCircle);
        for (size_t i = 0; i < n; ++i) {
            const Eigen::Vector3f &l = m_lights[i];
            Eigen::Vector3f h = (l + v).normalized();
            Eigen::Vector3f t = (std::abs(h.x()) < 0.9f)
                                    ? h.cross(Eigen::Vector3f(1, 0, 0)).normalized()
                                    : h.cross(Eigen::Vector3f(0, 1, 0)).normalized();
            Eigen::Vector3f b = h.cross(t);
            Eigen::Vector3f l_perp = l - l.dot(h) * h;
            float phi_d = 0.f;
            if (l_perp.norm() >= 1e-6f) {
                l_perp.normalize();
                phi_d = std::atan2(l_perp.dot(b), l_perp.dot(t)) * 180.f / float(M_PI);
            }
            series->append(double(phi_d), double(m_luminosities[i]));
        }
        {
            QVector<QPointF> snapPts = series->pointsVector();
            connect(series, &QScatterSeries::clicked, this, [this, snapPts](QPointF pt) {
                for (int i = 0; i < snapPts.size(); ++i) {
                    if (qAbs(snapPts[i].x() - pt.x()) < 1e-9 &&
                        qAbs(snapPts[i].y() - pt.y()) < 1e-9) {
                        emit lightSelected(i);
                        return;
                    }
                }
            });
        }
        chartPhiD->addSeries(series);
        auto *axisX = new QValueAxis();
        axisX->setRange(-180.0, 180.0);
        axisX->setTitleText("\u03c6_d (deg)");
        chartPhiD->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);
        auto *axisY = new QValueAxis();
        axisY->setRange(0, double(maxLum) * 1.05);
        axisY->setTitleText("Luminosity");
        chartPhiD->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
    }
}

void DiagnosticPanel::setNormal(const Eigen::Vector3f &n) {
    if (m_luminosities.empty() || m_lights.empty()) return;
    const size_t sz = m_luminosities.size();
    const float maxLum = *std::max_element(m_luminosities.begin(), m_luminosities.end());
    const Eigen::Vector3f nN = n.normalized();
    const Eigen::Vector3f v(0.f, 0.f, 1.f);

    auto fillAngle = [&](QChart *chart, const QString &xLabel,
                         std::function<float(size_t)> angleFn) {
        clearChart(chart);
        auto *series = new QScatterSeries();
        series->setMarkerSize(5.0);
        series->setMarkerShape(QScatterSeries::MarkerShapeCircle);
        for (size_t i = 0; i < sz; ++i)
            series->append(double(angleFn(i)), double(m_luminosities[i]));
        {
            QVector<QPointF> snapPts = series->pointsVector();
            connect(series, &QScatterSeries::clicked, this, [this, snapPts](QPointF pt) {
                for (int i = 0; i < snapPts.size(); ++i) {
                    if (qAbs(snapPts[i].x() - pt.x()) < 1e-9 &&
                        qAbs(snapPts[i].y() - pt.y()) < 1e-9) {
                        emit lightSelected(i);
                        return;
                    }
                }
            });
        }
        chart->addSeries(series);
        auto *axisX = new QValueAxis();
        axisX->setRange(0.0, 90.0);
        axisX->setTitleText(xLabel);
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);
        auto *axisY = new QValueAxis();
        axisY->setRange(0, double(maxLum) * 1.05);
        axisY->setTitleText("Luminosity");
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
    };

    // angle(L, N): diffuse — should follow cos(theta) / Lambert
    fillAngle(chartDiffuse, "angle(L, N)  (deg)", [&](size_t i) {
        return std::acos(std::clamp(m_lights[i].normalized().dot(nN), -1.f, 1.f))
               * 180.f / float(M_PI);
    });

    // angle(H, N): specular — drives the NDF peak
    fillAngle(chartSpecular, "angle(H, N)  (deg)", [&](size_t i) {
        Eigen::Vector3f h = (m_lights[i] + v).normalized();
        return std::acos(std::clamp(h.dot(nN), -1.f, 1.f))
               * 180.f / float(M_PI);
    });
}

// ---------------------------------------------------------------------------
// Rusinkiewicz reparameterization (fixed viewer at zenith = (0,0,1))
// ---------------------------------------------------------------------------
static void rusinkiewicz(const Eigen::Vector3f &l,
                         float &theta_h_deg, float &theta_d_deg, float &phi_d_deg)
{
    const Eigen::Vector3f v(0.f, 0.f, 1.f);
    Eigen::Vector3f h = (l + v).normalized();

    theta_h_deg = std::acos(std::clamp(h.z(), -1.f, 1.f)) * 180.f / float(M_PI);
    // NOTE: for a fixed viewer at zenith, theta_d == theta_h analytically
    // (l·h = h·z always). We compute theta_l (light elevation) instead,
    // which is independently meaningful.
    theta_d_deg = std::acos(std::clamp(l.z(), -1.f, 1.f)) * 180.f / float(M_PI);

    // Build a tangent frame around h to measure phi_d
    Eigen::Vector3f t = (std::abs(h.x()) < 0.9f)
                            ? h.cross(Eigen::Vector3f(1, 0, 0)).normalized()
                            : h.cross(Eigen::Vector3f(0, 1, 0)).normalized();
    Eigen::Vector3f b = h.cross(t);
    Eigen::Vector3f l_perp = l - l.dot(h) * h;
    if (l_perp.norm() < 1e-6f) {
        phi_d_deg = 0.f;
    } else {
        l_perp.normalize();
        phi_d_deg = std::atan2(l_perp.dot(b), l_perp.dot(t)) * 180.f / float(M_PI);
    }
}
