#ifndef LIGHTGEOMETRY_H
#define LIGHTGEOMETRY_H

#include "../src/dome.h"

#include <QFrame>
#include <QGraphicsScene>

class QLabel;
class QCheckBox;
class QDoubleSpinBox;
class QTextBrowser;
class QGraphicsView;

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
	void update(Dome dome);
	void setSpherical(int s);

private:
	void initLights();

	QTextBrowser *info = nullptr;
	QGraphicsView *lights = nullptr;
	QGraphicsScene scene;
};
#endif // LIGHTGEOMETRY_H
