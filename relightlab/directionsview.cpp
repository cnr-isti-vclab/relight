#include "directionsview.h"
#include "../src/dome.h"

#include <QGraphicsEllipseItem>

using namespace Eigen;

DirectionsView::DirectionsView(QWidget *parent): QGraphicsView(parent) {
	setScene(&scene);
}

void DirectionsView::initFromDome(Dome &dome) {
	scene.clear();

	qreal scale = width();
	//scene goes from [-1, +1]x[-1, +1], view will just zoom on it
	qreal diameter = lightSize;

	QPen pen(Qt::green, 1, Qt::DashLine);
	scene.addLine(0, -scale, 0, scale, pen);
	scene.addLine(-scale, 0, scale, 0, pen);


	int count = 0;
	std::vector<Vector3f> dirs;
	switch(dome.lightConfiguration) {
	case Dome::DIRECTIONAL: dirs = dome.directions; break;
	case Dome::SPHERICAL: dirs = dome.positionsSphere; break;
	case Dome::LIGHTS3D: dirs = dome.positions3d; break;
	}
	for(Vector3f dir: dirs) {
		dir.normalize();
		QGraphicsEllipseItem *e = scene.addEllipse(dir[0]*scale, -dir[1]*scale, diameter, diameter);
		e->setToolTip(QString::number(++count));
		e->setBrush(Qt::white);
	}

	qreal margin = scale/10;
	qreal side = scale + margin;
	fitInView(QRectF(-side, -side, 2*side, 2*side), Qt::KeepAspectRatio);
}

void DirectionsView::highlight(int n) {
	for(int i = 0; i < scene.items().size(); i++) {
		QGraphicsEllipseItem *item = (QGraphicsEllipseItem *)(scene.items()[i]);
		item->setBrush(i == n? Qt::red : Qt::white);
	}
}

void DirectionsView::clear() {
	scene.clear();
}
