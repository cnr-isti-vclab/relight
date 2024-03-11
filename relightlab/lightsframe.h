#ifndef LIGHTSFRAME_H
#define LIGHTSFRAME_H

#include "../src/dome.h"

#include <QStackedWidget>

class QLabel;
class QTabWidget;
class SpherePanel;
class DomePanel;
class LightsGeometry;

/* Dome holds the geometri config */

class LightsFrame: public QFrame {
	Q_OBJECT
public:
	LightsFrame();

public slots:
	void init();
	void init(Dome dome);

private:
	Dome dome;
	QTabWidget *choice = nullptr;
	SpherePanel *sphere_panel = nullptr;
	DomePanel *dome_panel = nullptr;
	LightsGeometry *geometry = nullptr;
};



#endif // LIGHTSFRAME_H
