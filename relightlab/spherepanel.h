#ifndef SPHEREPANEL_H
#define SPHEREPANEL_H

#include "../src/dome.h"

#include <QFrame>
#include <qdialog.h>

class SpherePicking;
class Sphere;

class SphereDialog: public QDialog {
	Q_OBJECT
public:
	SphereDialog(QWidget *parent = nullptr);
	void setSphere(Sphere *sphere);
public slots:
	void accept();
	void reject();

private:
	SpherePicking *sphere_picking = nullptr;
};


class SpherePanel: public QFrame {
	Q_OBJECT
public:
	SpherePanel(QWidget *parent = nullptr);
	void init();

public slots:
	void newSphere();

signals:
	void accept(Dome &dome);


private:
	Dome dome;
	SphereDialog *sphere_dialog = nullptr;
};



#endif // SPHEREPANEL_H
