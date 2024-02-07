#ifndef DOMEPANEL_H
#define DOMEPANEL_H

#include "../src/dome.h"

#include <QFrame>
#include <QGraphicsScene>

class QGraphicsView;

class DomePanel: public QFrame {
	Q_OBJECT
public:
	DomePanel(QWidget *parent = nullptr);
	void loadDome(int);
	void init();

signals:
	void accept(Dome dome);

private:
	QStringList domes;
	QGraphicsView *lights;
	QGraphicsScene scene;

	void initLights();
};

#endif // DOMEPANEL_H
