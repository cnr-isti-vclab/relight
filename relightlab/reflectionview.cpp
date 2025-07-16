#include "reflectionview.h"
#include "relightapp.h"
#include "../src/sphere.h"

#include <QGraphicsPixmapItem>
#include <QScrollBar>
#include <QRectF>

MarkerOverview::MarkerOverview(int _height, QWidget *parent): QGraphicsView(parent) {
	height = _height;
	setScene(&scene);

	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	QPixmap pix;
	img_item = scene.addPixmap(pix);
	if(qRelightApp->thumbnails().size())
		init();
}
void MarkerOverview::init() {
	QPixmap pix = QPixmap::fromImage(qRelightApp->thumbnails()[0]);
	img_item->setPixmap(pix);
	setFixedSize(pix.width()*height/pix.height(), height);
}

void MarkerOverview::resizeEvent(QResizeEvent */*event*/) {
	fitInView(scene.sceneRect(), Qt::KeepAspectRatio); //img_item);
}

SphereOverview::SphereOverview(QPointF _center, double _radius, int height, QWidget *parent):
	MarkerOverview(height, parent), center(_center), radius(_radius) {
	update();
}

void SphereOverview::update() {
//	scene.clear();
	if(ellipse)
		scene.removeItem(ellipse);

	QSizeF size = img_item->boundingRect().size();

	double scale = size.width()/(double)qRelightApp->project().imgsize.width();

	double r = radius*scale;
	QPointF scaled_center = center*scale;
	ellipse = scene.addEllipse(QRectF(scaled_center - QPointF(r, r), QSize(2*r, 2*r)), Qt::NoPen, Qt::green);
}

AlignOverview::AlignOverview(QRectF _rect, int height, QWidget *parent):
	MarkerOverview(height, parent), rect(_rect) {
	item = scene.addRect(QRectF(), Qt::NoPen, Qt::green);
	update();
}

void AlignOverview::update() {


	QSizeF size = img_item->boundingRect().size();
	/*img_item->setTransformOriginPoint(size.width() / 2, size.height() / 2);
	img_item->setRotation(angle); */

	double scale = size.width()/(double)qRelightApp->project().imgsize.width();
	QRectF r = QRectF(rect.x()*scale, rect.y()*scale, rect.width()*scale, rect.height()*scale);
	item->setRect(r);
}


ReflectionOverview::ReflectionOverview(Sphere *_sphere, int _height, QWidget *parent ): QGraphicsView(parent) {
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

ReflectionOverview::~ReflectionOverview() {
	for(auto l: lights) {
		scene.removeItem(l);
		delete l;
	}
	lights.clear();
}

void ReflectionOverview::init() {

	scene.clear();
	lights.clear();

	QPixmap pix = QPixmap::fromImage(sphere->sphereImg);
	img_item = scene.addPixmap(pix);
	setFixedSize(pix.width()*height/pix.height(), height);

	QPointF c = sphere->center - sphere->inner.topLeft();
	double w = sphere->radius;
	double h = sphere->radius;
	if(sphere->ellipse) {
		w = sphere->eWidth;
		h = sphere->eHeight;
	}

	w *= sphere->smallradius/sphere->radius;
	h *= sphere->smallradius/sphere->radius;
	area = scene.addEllipse(c.x() - w, c.y() - h, 2*w, 2*h, QPen(Qt::yellow));
	area->setTransformOriginPoint(c);
	area->setRotation(sphere->eAngle);
}

void ReflectionOverview::update() {
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

void ReflectionOverview::resizeEvent(QResizeEvent */*event*/) {
	fitInView(img_item->boundingRect(), Qt::KeepAspectRatio);
}


ZoomOverview::ZoomOverview(Crop _crop, int height, QWidget *parent):
	MarkerOverview(height, parent), crop(_crop) {
	QPolygonF p;
	item = scene.addRect(0, 0, 0, 0, QPen(Qt::green, 2), Qt::transparent);

	//item->setBrush(QBrush(Qt::transparent));
}

void ZoomOverview::update() {

	QSize img_size = qRelightApp->project().imgsize;

	QSize center = img_size/2;

	QPixmap pix= img_item->pixmap();
	QSize s = pix.size();
	QTransform transform;
	transform.rotate(crop.angle);
	QRect r(QPoint(0, 0), img_size - QSize(1, 1));
	QPolygon rotated = transform.map(QPolygon(r));
	QRect rotatedSize = rotated.boundingRect();

	QSize new_center = rotatedSize.size()/2;

	double scale = (double)img_size.width()/img_item->pixmap().width();
	QTransform t;
	t.translate(new_center.width(), new_center.height());
	t.rotate(crop.angle);
	t.translate(-center.width(), -center.height());
	t.scale(scale, scale);
	img_item->setTransform(t);

	item->setRect(crop);
	item->setPen(QPen(Qt::green, 2*scale));

	QRectF bound = img_item->mapToScene(img_item->boundingRect()).boundingRect();
	fitInView(bound,  Qt::KeepAspectRatio);
	QGraphicsView::update();
}


