#ifndef DIAGNOSTICPANEL_H
#define DIAGNOSTICPANEL_H

#include <QWidget>
#include <QtCharts>
#include <vector>
#include <Eigen/Core>

class DiagnosticPanel : public QWidget {
    Q_OBJECT
public:
    explicit DiagnosticPanel(QWidget *parent = nullptr);

    // Update the "By light" and φ_d charts from per-light luminosities and directions.
    // Stores lights/luminosities internally; clears the two normal-dependent charts.
    void update(const std::vector<float> &luminosities,
                const std::vector<Eigen::Vector3f> &lights);

    // Update the diffuse angle(L,N) and specular angle(H,N) charts once the
    // estimated normal is available.
    void setNormal(const Eigen::Vector3f &n);

signals:
    void lightSelected(int index);

private:
    static QChartView *makeChartView(QChart *chart, QWidget *parent);
    static void clearChart(QChart *chart);

    QChart *chartByLight  = nullptr;  // bar: luminosity by image index
    QChart *chartDiffuse  = nullptr;  // scatter: angle(L, N) — diffuse
    QChart *chartSpecular = nullptr;  // scatter: angle(H, N) — specular
    QChart *chartPhiD     = nullptr;  // scatter: φ_d azimuthal

    // Stored so they are available when setNormal() is called asynchronously
    std::vector<float>           m_luminosities;
    std::vector<Eigen::Vector3f> m_lights;
};

#endif // DIAGNOSTICPANEL_H

