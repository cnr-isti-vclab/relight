#ifndef MARKERDIALOG_H
#define MARKERDIALOG_H

#include <QDialog>

class SpherePicking;
class AlignPicking;
class Align;
class Sphere;

class MarkerDialog: public QDialog {
	Q_OBJECT
public:
	enum Marker { SPHERE, ALIGN };
	MarkerDialog(Marker marker, QWidget *parent = nullptr);
	void setAlign(Align *align);
	void setSphere(Sphere *sphere);
	QRectF getAlign();
	void clear();

public slots:
	void accept();
	void reject();

private:
	SpherePicking *sphere_picking = nullptr;
	AlignPicking *align_picking = nullptr;
};

#endif // SPHEREDIALOG_H
