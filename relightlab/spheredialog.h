#ifndef SPHEREDIALOG_H
#define SPHEREDIALOG_H

#include <QDialog>

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

#endif // SPHEREDIALOG_H
