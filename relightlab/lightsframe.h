#ifndef LIGHTSFRAME_H
#define LIGHTSFRAME_H

#include "../src/dome.h"
#include "domepanel.h"
#include "spherepanel.h"

#include <QStackedWidget>

class QLabel;
class LpFrame;
class DomeFrame;
class LightsGeometry;



class LightsFrame: public QFrame {
	Q_OBJECT
public:
	LightsFrame();

public slots:
	void init();
	void init(Dome dome);
	void exportDome();

private:
	Dome dome;
	SpherePanel *sphere_panel = nullptr;
	DomePanel *dome_panel = nullptr;
	LightsGeometry *geometry = nullptr;
};



#endif // LIGHTSFRAME_H
