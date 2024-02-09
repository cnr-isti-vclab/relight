#ifndef LIGHTGEOMETRY_H
#define LIGHTGEOMETRY_H

#include "../src/dome.h"

#include <QFrame>
#include <QGraphicsScene>
#include <QButtonGroup>

class QRadioButton;
class QDoubleSpinBox;

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
	void setSpherical(int s);
};
#endif // LIGHTGEOMETRY_H
