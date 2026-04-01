#ifndef RUSINVIEW_H
#define RUSINVIEW_H

#include <QWidget>
#include <Eigen/Core>
#include <vector>

// 2-D scatter in Rusinkiewicz space:  x = θ_h (0–90°),  y = θ_d (0–90°).
// Each light is rendered as a dot coloured by its actual RGB reflectance value.
class RusinkiewiczView : public QWidget {
    Q_OBJECT
public:
    explicit RusinkiewiczView(QWidget *parent = nullptr);

    // Supply RGB colors + light directions.  Uses n=(0,0,1) until setNormal() is called.
    void setData(const std::vector<Eigen::Vector3f> &colors,
                 const std::vector<Eigen::Vector3f> &lights);

    // Recompute angles with the given surface normal and repaint.
    void setNormal(const Eigen::Vector3f &n);

    void clear();

    QSize sizeHint() const override        { return {480, 360}; }
    QSize minimumSizeHint() const override { return {240, 180}; }

signals:
    void lightSelected(int index);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void recomputeAngles();

    std::vector<Eigen::Vector3f> m_colors;
    std::vector<Eigen::Vector3f> m_lights;
    Eigen::Vector3f              m_normal{0.f, 0.f, 1.f};
    bool                         m_hasData = false;

    std::vector<float> m_thetaH;  // degrees, one per light
    std::vector<float> m_thetaD;  // degrees, one per light
};

#endif // RUSINVIEW_H
