#ifndef LIGHTSFRAME_H
#define LIGHTSFRAME_H

#include "../src/dome.h"

#include <QStackedWidget>

class QLabel;
class QTabWidget;
class SpherePanel;
class DomePanel;
class LightsGeometry;

/* Dome holds the geometri config 
   SpherePanel uses the reflective spheres to calculate the light direction. (stored in the project)*/


class LightsFrame: public QFrame {
	Q_OBJECT
public:
	LightsFrame();

public slots:
	void clear();
	void init();

private:
	QTabWidget *choice = nullptr;
	SpherePanel *sphere_panel = nullptr;
	DomePanel *dome_panel = nullptr;
	LightsGeometry *geometry = nullptr;
};



#endif // LIGHTSFRAME_H
