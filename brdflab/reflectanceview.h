#ifndef REFLECTANCEVIEW_H
#define REFLECTANCEVIEW_H

#include <QWidget>
#include <Eigen/Core>
#include <vector>

// Top-down (lx, ly) view of all light samples.
// Each sample is rendered as a dot at the light's (x,y) position in the unit
// disc, coloured by the actual RGB reflectance value for that light direction.
class ReflectanceView : public QWidget {
    Q_OBJECT
public:
    explicit ReflectanceView(QWidget *parent = nullptr);

    // Supply RGB colors (per-light) + light directions.
    void setData(const std::vector<Eigen::Vector3f> &colors,
                 const std::vector<Eigen::Vector3f> &lights);

    void clear();

    QSize sizeHint()        const override { return {360, 360}; }
    QSize minimumSizeHint() const override { return {180, 180}; }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    struct Layout { double cx, cy, r; };
    Layout layout() const;

    std::vector<Eigen::Vector3f> m_colors;
    std::vector<Eigen::Vector3f> m_lights;
    bool m_hasData = false;
};

#endif // REFLECTANCEVIEW_H
