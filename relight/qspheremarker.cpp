#include "qspheremarker.h"
#include "sphere.h"
#include "mainwindow.h" //for RTIScene
#include <QGraphicsEllipseItem>
#include <QPen>
#include <QLabel>
#include <QApplication>

BorderPoint::~BorderPoint() {}

QVariant BorderPoint::itemChange(GraphicsItemChange change, const QVariant &value)	{
	if ((change == ItemPositionChange  && scene()) || change == ItemScenePositionHasChanged) {
		RTIScene *s = (RTIScene *)scene();
		emit s->borderPointMoved(this);
	}
	return QGraphicsItem::itemChange(change, value);
}


HighlightPoint::~HighlightPoint() {}

QVariant HighlightPoint::itemChange(GraphicsItemChange change, const QVariant &value)	{
	if ((change == ItemPositionChange  && scene()) || change == ItemScenePositionHasChanged) {
		RTIScene *s = (RTIScene *)scene();
		emit s->highlightMoved(this);
	}
	return QGraphicsItem::itemChange(change, value);
}


SphereMarker::SphereMarker( Sphere *s, QGraphicsView *_view, QWidget *parent):
	Marker(_view, parent), sphere(s) {

	label->setText("Sphere");

	QPen outlinePen(Qt::yellow);
	outlinePen.setCosmetic(true);
	circle = new QGraphicsEllipseItem(0, 0, 1, 1);
	circle->setPen(outlinePen);

	axis[0] = new QGraphicsLineItem(0, 0, 1, 1);
	axis[1] = new QGraphicsLineItem(0, 0, 1, 1);

	QVector<qreal> dashes;
	dashes << 4 << 4;
	outlinePen.setDashPattern(dashes);
	smallcircle = new QGraphicsEllipseItem(0, 0, 1, 1);
	smallcircle->setPen(outlinePen);

	highlight = new HighlightPoint(-4, -4, 4, 4);

	QPen pen(Qt::red);
	pen.setWidth(2);
	pen.setCosmetic(true);
	highlight->setPen(pen);
	highlight->setBrush(Qt::red);
	highlight->setBrush(Qt::green);
	highlight->setFlag(QGraphicsItem::ItemIsMovable);
	highlight->setFlag(QGraphicsItem::ItemIsSelectable);
	highlight->setFlag(QGraphicsItem::ItemSendsScenePositionChanges);

	scene->addItem(circle);
	scene->addItem(smallcircle);
	scene->addItem(highlight);
	scene->addItem(axis[0]);
	scene->addItem(axis[1]);

	init();
	for(QPointF pos: sphere->border)
		addBorderPoint(pos);


}

SphereMarker::~SphereMarker() {
	delete circle;
	delete smallcircle;
	delete highlight;
	for(auto *b: border)
		delete b;
}

void SphereMarker::init() {
	circle->setVisible(false);
	smallcircle->setVisible(false);

	if(sphere->center.isNull())
		return;

	QPointF c = sphere->center;
	double R = double(sphere->radius);
	double r = double(sphere->smallradius);
	if(sphere->ellipse) {
		circle->setRect(c.x() - sphere->eWidth, c.y() - sphere->eHeight, 2*sphere->eWidth, 2*sphere->eHeight);
		circle->setTransformOriginPoint(c);
		circle->setRotation(sphere->eAngle);
		QPointF dir = { cos(sphere->eAngle*M_PI/180), sin(sphere->eAngle*M_PI/180) };
		axis[0]->setLine(c.x(), c.y(), c.x() + dir.x()*sphere->eWidth, c.y() + dir.y()*sphere->eWidth);
		axis[1]->setLine(c.x(), c.y(), c.x() - dir.y()*sphere->eHeight, c.y() + dir.x()*sphere->eHeight);
	} else {
		circle->setRect(c.x()-R, c.y()-R, 2*R, 2*R);
	}
	circle->setVisible(true);
	smallcircle->setRect(c.x()-r, c.y()-r, 2*r, 2*r);
	smallcircle->setVisible(true);
}

void SphereMarker::fit() {
	if(sphere->border.size() < 3)
		sphere->center = QPointF();
	else
		sphere->fit();

	init();
}


void SphereMarker::click(QPointF pos) {
	//min distance between border points in pixels.
	double minBorderDist = 20;
	for(QPointF p: sphere->border) {
		if(sqrt(pow(p.x() - pos.x(), 2) + pow(p.y() - pos.y(), 2)) < minBorderDist)
			return;
	}

	addBorderPoint(pos);
	sphere->border.push_back(pos);
	fit();
}

void SphereMarker::addBorderPoint(QPointF pos) {

	QBrush blueBrush(Qt::white);
	QPen outlinePen(Qt::white);
	outlinePen.setWidth(5);
	outlinePen.setCosmetic(true);

	auto borderPoint = new BorderPoint(-3, -3, 6, 6);
	borderPoint->setPos(pos);
	borderPoint->setPen(outlinePen);
	borderPoint->setBrush(blueBrush);
	border.push_back(borderPoint);
	scene->addItem(borderPoint);
}
void SphereMarker::updateBorderPoint(QGraphicsEllipseItem *point) {
	for(size_t i = 0; i < border.size(); i++) {
		if(point != border[i])
			continue;
		sphere->border[i] = point->pos();
		break;
	}
}


void SphereMarker::setSelected(bool value) {
	QPen pen = circle->pen();
	pen.setWidth(value? 2 : 1);
	circle->setPen(pen);

	pen = smallcircle->pen();
	pen.setWidth(value? 2 : 1);
	smallcircle->setPen(pen);

	Marker::setSelected(value);
}

void SphereMarker::showHighlight(size_t n) {
	highlight->setVisible(sphere->fitted);

	if(!sphere->fitted)
		return;

	//QRectF mark(- QPointF(4, 4), QPointF(4, 4));
	//sphere->highlight->setRect(mark);
	highlight->setVisible(true);
	QPen pen = highlight->pen();

	if(!sphere->lights[n].isNull()) {
		highlight->setPos(sphere->lights[n]);
		highlight->setBrush(Qt::green);
		pen.setColor(Qt::green);
	} else {
		highlight->setPos(sphere->inner.center());
		highlight->setBrush(Qt::red);
		pen.setColor(Qt::red);
	}
	highlight->setPen(pen);
}

void SphereMarker::updateHighlightPosition(size_t n) {
	highlight->setBrush(Qt::green);
	QPen pen = highlight->pen();
	pen.setColor(Qt::green);
	highlight->setPen(pen);
	sphere->lights[n] = highlight->pos();
}

void SphereMarker::deleteSelected(int currentImage) {
	size_t j = 0;
	for(size_t i = 0; i < border.size(); i++, j++) {
		if(i != j) {
			border[j] = border[i];
			sphere->border[j] = sphere->border[i];
		}
		if(border[i]->isSelected()) {
			delete border[i];
			j--;
		}
	}
	border.resize(j);
	sphere->border.resize(j);

	if(highlight->isSelected()) {
		sphere->resetHighlight(currentImage);
		showHighlight(currentImage);
	}
	fit();
}

