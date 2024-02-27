#ifndef LIGHTGEOMETRY_H
#define LIGHTGEOMETRY_H

#include "../src/dome.h"

#include <QFrame>

class QRadioButton;
class QDoubleSpinBox;
class QButtonGroup;
class QAbstractButton;

class LightsGeometry: public QFrame {
	Q_OBJECT
public:	
	QDoubleSpinBox *image_width;

	QRadioButton *directional;
	QRadioButton *sphere_approx;
	QRadioButton *three;

	QDoubleSpinBox *vertical_offset;
	QDoubleSpinBox *diameter;

	QButtonGroup *group = nullptr;

	LightsGeometry(QWidget *parent = nullptr);
	~LightsGeometry() { if(group) delete group; }

	void init();

public slots:
	void update(Dome dome);
	void setSpherical(QAbstractButton *button);
};
#endif // LIGHTGEOMETRY_H
