#ifndef LIGHTGEOMETRY_H
#define LIGHTGEOMETRY_H

#include "../src/dome.h"

#include <QFrame>

class QDoubleSpinBox;
class QButtonGroup;
class QAbstractButton;
class HelpRadio;

class LightsGeometry: public QFrame {
	Q_OBJECT
public:	
	QDoubleSpinBox *image_width;

	HelpRadio *directional;
	HelpRadio *sphere_approx;
	HelpRadio *three;

	QDoubleSpinBox *vertical_offset;
	QDoubleSpinBox *diameter;

	QButtonGroup *group = nullptr;

	LightsGeometry(QWidget *parent = nullptr);
	~LightsGeometry();

	void init();

public slots:
	void update(Dome dome);
	void setSpherical(QAbstractButton *button);
};
#endif // LIGHTGEOMETRY_H
