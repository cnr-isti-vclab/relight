#ifndef LIGHTGEOMETRY_H
#define LIGHTGEOMETRY_H


#include <QFrame>

class QLabel;
class QCheckBox;
class QDoubleSpinBox;

class LightsGeometry: public QFrame {
	Q_OBJECT
public:
	QLabel *images_number;
	QCheckBox *sphere_approx;
	QDoubleSpinBox *image_width;
	QDoubleSpinBox *vertical_offset;
	QDoubleSpinBox *diameter;

	LightsGeometry(QWidget *parent = nullptr);
	void init();

public slots:
	void setSpherical(int s);
};
#endif // LIGHTGEOMETRY_H
