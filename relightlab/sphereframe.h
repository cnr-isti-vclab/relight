#ifndef SPHEREPANEL_H
#define SPHEREPANEL_H

#include <QGroupBox>
#include <qdialog.h>
#include "../src/dome.h"
#include "../src/task.h"
#include "../src/spherelocator.h"

class MarkerDialog;
class QStackedWidget;
class Sphere;
class QVBoxLayout;
class SphereRow;
class Dome;
class QPushButton;
class QProgressBar;

class LocateSpheres: public Task {
public:
	std::vector<SphereLocator::Circle> circles;

	LocateSpheres();
	virtual void run() override;
};

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
	void locateSpheres();
	void locateProgress(QString msg, int percent);
	void locateFinished();

signals:
	void updated();

private:
	QStackedWidget *stack = nullptr;
	MarkerDialog *marker_dialog = nullptr;

	Sphere *provisional_sphere = nullptr;
	QVBoxLayout *spheres = nullptr;
	QPushButton *locate_button = nullptr;
	QProgressBar *locate_progress = nullptr;
	LocateSpheres *locate_spheres = nullptr;

	SphereRow *findRow(Sphere *sphere);

};



#endif // SPHEREPANEL_H
