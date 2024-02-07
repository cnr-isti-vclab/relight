#ifndef LIGHTSFRAME_H
#define LIGHTSFRAME_H

#include "lppanel.h"
#include "domepanel.h"

#include <QStackedWidget>

class QLabel;
class LpFrame;
class DomeFrame;
class LightsGeometry;

class Card: public QFrame {
	Q_OBJECT
public:
	Card(QString title, QString subtitle, QWidget *parent = nullptr);
public slots:
	void click() { emit clicked(); }
signals:
	void clicked();
};

class SpherePanel: public QFrame {
	Q_OBJECT
public:
	SpherePanel(QWidget *parent = nullptr);
	void init();
signals:
	void accept(Dome &dome);
};



class LightsFrame: public QFrame {
	Q_OBJECT
public:
	LightsFrame();

public slots:
	void init();

private:
	LpPanel *lp = nullptr;
	SpherePanel *sphere = nullptr;
	DomePanel *dome = nullptr;
	LightsGeometry *geometry = nullptr;
};



#endif // LIGHTSFRAME_H
