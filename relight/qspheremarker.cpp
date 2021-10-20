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


QSphereMarker::QSphereMarker( Sphere *s, QGraphicsView *_view, QWidget *parent):
	QMarker(_view, parent), sphere(s) {

	label->setText("Sphere");

	QPen outlinePen(Qt::yellow);
	outlinePen.setCosmetic(true);
	circle = new QGraphicsEllipseItem(0, 0, 1, 1);
	circle->setPen(outlinePen);

	QVector<qreal> dashes;
	dashes << 4 << 4;
	outlinePen.setDashPattern(dashes);
	smallcircle = new QGraphicsEllipseItem(0, 0, 1, 1);
	smallcircle->setPen(outlinePen);

	highlight = new HighlightPoint(-4, -4, 4, 4);

	QPen pen;
	pen.setColor(Qt::transparent);
	pen.setWidth(0);
	highlight->setPen(pen);
	highlight->setBrush(Qt::green);
	highlight->setFlag(QGraphicsItem::ItemIsMovable);
	highlight->setFlag(QGraphicsItem::ItemIsSelectable);
	highlight->setFlag(QGraphicsItem::ItemSendsScenePositionChanges);

	scene->addItem(circle);
	scene->addItem(smallcircle);
	scene->addItem(highlight);

	init();
	for(QPointF pos: sphere->border)
		addBorderPoint(pos);


}

QSphereMarker::~QSphereMarker() {
	delete circle;
	delete smallcircle;
	delete highlight;
	for(auto *b: border)
		delete b;
}

void QSphereMarker::init() {
	circle->setVisible(false);
	smallcircle->setVisible(false);

	if(sphere->center.isNull())
		return;

	QPointF c = sphere->center;
	double R = double(sphere->radius);
	double r = double(sphere->smallradius);
	circle->setRect(c.x()-R, c.y()-R, 2*R, 2*R);
	circle->setVisible(true);
	smallcircle->setRect(c.x()-r, c.y()-r, 2*r, 2*r);
	smallcircle->setVisible(true);
}

void QSphereMarker::fit(QSize imagesize = QSize(0, 0)) {
	if(sphere->border.size() < 3)
		sphere->center = QPointF();
	else
		sphere->fit(imagesize);

	init();
}

void QSphereMarker::setEditing(bool value) {
	if(value)
		QApplication::changeOverrideCursor(Qt::CrossCursor);
	else
		QApplication::restoreOverrideCursor();

	QMarker::setEditing(value);
}

void QSphereMarker::click(QPointF pos) {
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

void QSphereMarker::addBorderPoint(QPointF pos) {

	QBrush blueBrush(Qt::white);
	QPen outlinePen(Qt::white);
	outlinePen.setWidth(5);
	outlinePen.setCosmetic(true);

	auto borderPoint = new BorderPoint(-3, -3, 6, 6);
	borderPoint->setPos(pos);
	borderPoint->setPen(outlinePen);
	borderPoint->setBrush(blueBrush);
	borderPoint->setFlag(QGraphicsItem::ItemIsMovable);
	borderPoint->setFlag(QGraphicsItem::ItemIsSelectable);
	borderPoint->setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
	borderPoint->setCursor(Qt::CrossCursor);
	border.push_back(borderPoint);
	scene->addItem(borderPoint);
}
void QSphereMarker::updateBorderPoint(QGraphicsEllipseItem *point) {
	for(int i = 0; i < border.size(); i++) {
		if(point != border[i])
			continue;
		sphere->border[i] = point->pos();
		break;
	}
}


void QSphereMarker::setSelected(bool value) {
	QPen pen = circle->pen();
	pen.setWidth(value? 2 : 1);
	circle->setPen(pen);

	pen = smallcircle->pen();
	pen.setWidth(value? 2 : 1);
	smallcircle->setPen(pen);

	QMarker::setSelected(value);
}

void QSphereMarker::showHighlight(size_t n) {
	if(!sphere->fitted)
		return;

	//QRectF mark(- QPointF(4, 4), QPointF(4, 4));
	//sphere->highlight->setRect(mark);
	highlight->setVisible(true);

	if(!sphere->lights[n].isNull()) {
		highlight->setPos(sphere->lights[n]);
		highlight->setBrush(Qt::green);
	} else {
		highlight->setPos(sphere->inner.center());
		highlight->setBrush(Qt::red);
	}
}

void QSphereMarker::updateHighlightPosition(size_t n) {
	highlight->setBrush(Qt::green);
	sphere->lights[n] = highlight->pos();
}

void QSphereMarker::deleteSelected() {
	return;
		/*auto border = sphere->border;
		sphere->border.clear();
		std::copy_if (border.begin(), border.end(), std::back_inserter(sphere->border), [border](QGraphicsEllipseItem *e) {
			bool remove = e->isSelected();
			if(remove) delete e;
			return !remove;
		});
		if(highlight->isSelected()) {
			sphere->resetHighlight(currentImage);
			QStandardItem *item = imageModel->item(currentImage);
			item->setBackground(QBrush());

			showHighlights(currentImage);
		}
	}
	updateBorderPoints(); */
}

