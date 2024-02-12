#include "directionsview.h"
#include "../src/dome.h"

#include <QGraphicsEllipseItem>
#include <QDebug>

DirectionsView::DirectionsView(QWidget *parent): QGraphicsView(parent) {
	setScene(&scene);
}

void DirectionsView::initFromDome(Dome &dome) {
	qreal scale = width();
	//scene goes from [-1, +1]x[-1, +1], view will just zoom on it
	qreal diameter = lightSize;
	int count = 0;
	for(Vector3f &dir: dome.directions) {
		QGraphicsEllipseItem *e = scene.addEllipse(dir[0]*scale, dir[1]*scale, diameter, diameter);
		e->setToolTip(QString::number(count++));
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
