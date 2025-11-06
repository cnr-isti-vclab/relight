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

/* This class display the current dome parameters (see dome.h for details
 *
 * The dome is computed using:
 * 1) reflective spheres. The project computes the dome using the spheres.
 * 2) lp: again the dome is computed on load, but no 3d positions.
 */

class LightsGeometry: public QFrame {
	Q_OBJECT
public:	
	QSpinBox *images_number;
	QTextEdit *notes;
	QLineEdit *filename;
	HelpRadio *directional;
	HelpRadio *sphere_approx;
	HelpRadio *lights3d;

	QDoubleSpinBox *image_width;
	QDoubleSpinBox *vertical_offset;
	QDoubleSpinBox *radius;
	DirectionsView *directions_view;

	QButtonGroup *group = nullptr;

	LightsGeometry(QWidget *parent = nullptr);
	~LightsGeometry();

public slots:
	void init();
	void setFromSpheres();              //when reflective spheres reflections have been processed
	void setSpherical(QAbstractButton *button);
	void exportDome();
	void recomputeGeometry();           //recompute positions when geometry parameters change
};
#endif // LIGHTGEOMETRY_H
