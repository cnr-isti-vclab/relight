#include "reflectionview.h"
#include "../src/sphere.h"

#include <QGraphicsPixmapItem>

ReflectionView::ReflectionView(Sphere *_sphere, QWidget *parent ): QGraphicsView(parent) {
	sphere = _sphere;
	update();
}

void ReflectionView::update() {
	scene.clear();

	QPixmap pix = QPixmap::fromImage(sphere->sphereImg);
	img_item = scene.addPixmap(pix);

	for(QPointF &p: sphere->lights) {
		double side = lightSize/2;
		scene.addEllipse(p.x()-side, p.y() - side, 2*side, 2*side, QPen(), Qt::green);
	}
	fitInView(img_item);
}


