#include "reflectionview.h"
#include "relightapp.h"
#include "../src/sphere.h"

#include <QGraphicsPixmapItem>
#include <QScrollBar>
#include <QRectF>
#include <QDebug>

PositionView::PositionView(Sphere *_sphere, int _height, QWidget *parent): QGraphicsView(parent) {
	sphere = _sphere;
	height = _height;
	setScene(&scene);

	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	QPixmap pix = QPixmap::fromImage(qRelightApp->thumbnails()[0]);
	img_item = scene.addPixmap(pix);

	setFixedSize(pix.width()*height/pix.height(), height);

	update();
}

void PositionView::resizeEvent(QResizeEvent */*event*/) {
	fitInView(scene.sceneRect()); //img_item);
}

void PositionView::update() {
//	scene.clear();
	if(ellipse)
		scene.removeItem(ellipse);

	QSizeF size = img_item->boundingRect().size();

	double scale = size.width()/(double)qRelightApp->project().imgsize.width();

	double radius = sphere->radius*scale;
	QPointF scaled_center = sphere->center*scale;
	ellipse = scene.addEllipse(QRectF(scaled_center - QPointF(radius, radius), QSize(2*radius, 2*radius)), Qt::NoPen, Qt::green);
}


ReflectionView::ReflectionView(Sphere *_sphere, int _height, QWidget *parent ): QGraphicsView(parent) {
	sphere = _sphere;
	height = _height;
	setScene(&scene);

	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	verticalScrollBar()->blockSignals(true);
	horizontalScrollBar()->blockSignals(true);

	init();
	update();
}

ReflectionView::~ReflectionView() {
	for(auto l: lights) {
		scene.removeItem(l);
		delete l;
	}
	lights.clear();
}

void ReflectionView::init() {
	scene.clear();
	lights.clear();

	QPixmap pix = QPixmap::fromImage(sphere->sphereImg);//.scaledToHeight(height);
	img_item = scene.addPixmap(pix);
	setFixedSize(pix.width()*height/pix.height(), height);
	//setFixedSize(pix.width(), pix.height());

	QPointF c = sphere->center - sphere->inner.topLeft();
	double w = sphere->eWidth;
	double h = sphere->eHeight;

	w *= sphere->smallradius/sphere->radius;
	h *= sphere->smallradius/sphere->radius;
	area = scene.addEllipse(c.x() - w, c.y() - h, 2*w, 2*h, QPen(Qt::yellow));
	area->setTransformOriginPoint(c);
	area->setRotation(sphere->eAngle);
}

void ReflectionView::update() {
	for(auto l: lights) {
		scene.removeItem(l);
		delete l;
	}
	lights.clear();

	QPixmap pix = QPixmap::fromImage(sphere->sphereImg);
	double scale = transform().m11();
	img_item->setPixmap(pix);

	double side = lightRadius/scale;
	for(QPointF p: sphere->lights) {
		if(p.isNull())
			continue;
		p.setX(p.x() - sphere->inner.left());
		p.setY(p.y() - sphere->inner.top());

		auto ellipse = scene.addEllipse(p.x()-side, p.y() - side, 2*side, 2*side, Qt::NoPen, Qt::green);
		lights.push_back(ellipse);
	}
}

void ReflectionView::resizeEvent(QResizeEvent *event) {
	fitInView(img_item->boundingRect()); //.sceneRect()); //img_item);
}


