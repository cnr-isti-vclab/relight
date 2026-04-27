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

void MarkerOverview::setImage(const QPixmap &pix) {
	img_item->setPixmap(pix);
	img_item->setTransform(QTransform());
	setFixedSize(pix.width()*height/pix.height(), height);
	fitInView(img_item->boundingRect(), Qt::KeepAspectRatio);
	QGraphicsView::update();
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
	if(img_item->pixmap().isNull()) return;
	item->setVisible(true);

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

void ZoomOverview::showNormalmap(const QString &path) {
	QPixmap pix(path);
	if(pix.isNull()) return;
	item->setVisible(false);
	setImage(pix);
}

// ── PlaneOverview ─────────────────────────────────────────────────────────────

PlaneOverview::PlaneOverview(int height, QWidget *parent)
	: MarkerOverview(height, parent)
{
}

void PlaneOverview::setPoints(const std::vector<QPointF> &pts)
{
	points = pts;
	update();
}

void PlaneOverview::update()
{
	// Remove old dots from scene.
	for(QGraphicsEllipseItem *dot : dot_items) {
		scene.removeItem(dot);
		delete dot;
	}
	dot_items.clear();

	if(points.empty())
		return;

	// Use the pixmap bounding rect (same as SphereOverview) so the scale is
	// always relative to the actual loaded thumbnail, not the raw image size.
	QSizeF pix_size = img_item->boundingRect().size();
	if(pix_size.isEmpty())
		return;

	QSize img_size = qRelightApp->project().imgsize;
	double scale_x = pix_size.width()  / std::max(1, img_size.width());
	double scale_y = pix_size.height() / std::max(1, img_size.height());

	QPen pen(Qt::green);
	pen.setCosmetic(true);
	pen.setWidth(2);

	for(const QPointF &p : points) {
		double sx = p.x() * scale_x;
		double sy = p.y() * scale_y;
		QGraphicsEllipseItem *dot = scene.addEllipse(sx - 4, sy - 4, 8, 8, pen, QBrush(QColor(0, 200, 0, 160)));
		dot_items.push_back(dot);
	}

	// Fit the view to the image bounds (not the whole scene which includes dots).
	fitInView(img_item->boundingRect(), Qt::KeepAspectRatio);
	QGraphicsView::update();
}