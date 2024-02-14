#ifndef SPHEREPANEL_H
#define SPHEREPANEL_H

#include "../src/dome.h"
#include "../relight/task.h"
#include <QFrame>
#include <qdialog.h>


class SphereDialog;
class Sphere;
class QVBoxLayout;
class SphereRow;

class SpherePanel: public QFrame {
	Q_OBJECT
public:
	SpherePanel(QWidget *parent = nullptr);
	void init();
	SphereRow *addSphere(Sphere *sphere);


public slots:
	void newSphere();
	void removeSphere(SphereRow *sphere);

signals:
	void accept(Dome dome);

private:
	Dome dome;
	SphereDialog *sphere_dialog = nullptr;
	QVBoxLayout *spheres = nullptr;
};



#endif // SPHEREPANEL_H
