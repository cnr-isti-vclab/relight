#ifndef NORMALSPHEREWIDGET_H
#define NORMALSPHEREWIDGET_H

#include <QWidget>
#include <Eigen/Core>

// Displays the estimated surface normal as a dot on a rendered hemisphere.
// The dot is draggable; releasing emits normalChanged().
class NormalSphereWidget : public QWidget {
    Q_OBJECT
public:
    explicit NormalSphereWidget(QWidget *parent = nullptr);

    void setNormal(const Eigen::Vector3f &n);
    void clear();

    QSize sizeHint() const override { return {260, 280}; }
    QSize minimumSizeHint() const override { return {160, 170}; }

signals:
    void normalChanged(const Eigen::Vector3f &n);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    // Returns {cx, cy, r} for the current widget size
    struct Layout { double cx, cy, r; };
    Layout layout() const;

    Eigen::Vector3f normalFromPoint(double px, double py) const;

    bool  hasNormal  = false;
    bool  dragging   = false;
    Eigen::Vector3f normal{0, 0, 1};
};

#endif // NORMALSPHEREWIDGET_H
