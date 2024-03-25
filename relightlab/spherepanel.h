#ifndef SPHEREPANEL_H
#define SPHEREPANEL_H

#include <QFrame>
#include <qdialog.h>
#include "../src/dome.h"

class SphereDialog;
class Sphere;
class QVBoxLayout;
class SphereRow;
class Dome;


class SpherePanel: public QFrame {
	Q_OBJECT
public:
	SpherePanel(QWidget *parent = nullptr);
	void clear();
	void init();
	SphereRow *addSphere(Sphere *sphere);

public slots:
	void newSphere();
	void removeSphere(SphereRow *sphere);

signals:
	void updated();

private:
	SphereDialog *sphere_dialog = nullptr;
	QVBoxLayout *spheres = nullptr;
};



#endif // SPHEREPANEL_H
