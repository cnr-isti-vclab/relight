#ifndef LIGHTGEOMETRY_H
#define LIGHTGEOMETRY_H

#include "../src/dome.h"
#include <QFrame>

class QDoubleSpinBox;
class QButtonGroup;
class QSpinBox;
class QTextEdit;
class QLineEdit;
class QAbstractButton;
class HelpRadio;
class DirectionsView;

class LightsGeometry: public QFrame {
	Q_OBJECT
public:	
	QDoubleSpinBox *image_width;
	QSpinBox *images_number;
	QTextEdit *notes;
	QLineEdit *filename;
	HelpRadio *directional;
	HelpRadio *sphere_approx;
	HelpRadio *lights3d;

	QDoubleSpinBox *vertical_offset;
	QDoubleSpinBox *diameter;
	DirectionsView *directions_view;

	QButtonGroup *group = nullptr;

	LightsGeometry(QWidget *parent = nullptr);
	~LightsGeometry();

	void init();


public slots:
	void setDome(Dome dome);        //when a dome is selected
	void setSpheres();              //when reflective spheres reflections have been processed
	void setSpherical(QAbstractButton *button);
	void exportDome();
};
#endif // LIGHTGEOMETRY_H
