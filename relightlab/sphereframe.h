#ifndef SPHEREPANEL_H
#define SPHEREPANEL_H

#include <QGroupBox>
#include <qdialog.h>
#include "../src/dome.h"

class MarkerDialog;
class QStackedWidget;
class Sphere;
class QVBoxLayout;
class SphereRow;
class Dome;


class SphereFrame: public QGroupBox {
	Q_OBJECT
public:
	SphereFrame(QWidget *parent = nullptr);
	void clear();
	void init();
	SphereRow *addSphere(Sphere *sphere);

public slots:
	void newSphere();
	void editSphere(SphereRow *sphere);
	void removeSphere(SphereRow *sphere);
	void okMarker();
	void cancelMarker();

signals:
	void updated();

private:
	QStackedWidget *stack = nullptr;
	MarkerDialog *marker_dialog = nullptr;

	Sphere *provisional_sphere = nullptr;
	QVBoxLayout *spheres = nullptr;

	SphereRow *findRow(Sphere *sphere);

};



#endif // SPHEREPANEL_H
