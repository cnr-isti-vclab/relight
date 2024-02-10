#include "spherepicking.h"
#include "canvas.h"
#include "../src/sphere.h"

#include <QGraphicsEllipseItem>
#include <QPen>
#include <QLabel>
#include <QKeyEvent>
#include <QToolBar>
#include <QStatusBar>

SpherePicking::SpherePicking(QWidget *parent): ImageFrame(parent) {
	left_toolbar->hide();
	right_toolbar->hide();

	status->showMessage("Double click on the boundary of the sphere.");


	//create shpere marking items.
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

	highlight = new HighlightPoint(this, -4, -4, 4, 4);

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

	connect(canvas, SIGNAL(clicked(QPoint)), this, SLOT(click(QPoint)));
}

void SpherePicking::showImage(int id) {
	ImageFrame::showImage(id);

	status->showMessage("Double click on the boundary of the sphere.");
}


/*void SpherePicking::updatePoint(QGraphicsEllipseItem *point) {
	updatePoint(point);
	sphere->fit();
		int sphere_id = project.indexOf(m->sphere);
		assert(sphere_id != -1);
		history.push(Action(Action::MOVE_BORDER, sphere_id, *(m->sphere)));
	}
}*/




BorderPoint::~BorderPoint() {}

QVariant BorderPoint::itemChange(GraphicsItemChange change, const QVariant &value)	{
	if ((change == ItemPositionChange  && scene()) || change == ItemScenePositionHasChanged) {
		picker->updateBorderPoints();
	}
	return QGraphicsItem::itemChange(change, value);
}


HighlightPoint::~HighlightPoint() {}

QVariant HighlightPoint::itemChange(GraphicsItemChange change, const QVariant &value)	{
	if ((change == ItemPositionChange  && scene()) || change == ItemScenePositionHasChanged) {
		picker->updateHighlightPosition();
	}
	return QGraphicsItem::itemChange(change, value);
}

void SpherePicking::clear() {
	circle->setVisible(false);
	smallcircle->setVisible(false);
	axis[0]->setVisible(false);
	axis[1]->setVisible(false);
	highlight->setVisible(false);
	for(BorderPoint *b: border) {
		scene->removeItem(b);
		delete b;
	}
	border.clear();
}

void SpherePicking::setSphere(Sphere *s) {
	sphere = s;

	//TODO uupdatescene, not set sphere
	clear();

	for(QPointF pos: sphere->border)
		addBorderPoint(pos);

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


void SpherePicking::fit() {
	if(sphere->border.size() < 3)
		sphere->center = QPointF();
	else
		sphere->fit();

	setSphere(sphere);
}


void SpherePicking::click(QPoint p) {
	QPointF pos = canvas->mapToScene(p);

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

void SpherePicking::addBorderPoint(QPointF pos) {

	QBrush blueBrush(Qt::white);
	QPen outlinePen(Qt::white);
	outlinePen.setWidth(5);
	outlinePen.setCosmetic(true);

	auto borderPoint = new BorderPoint(this, -3, -3, 6, 6);
	borderPoint->setPos(pos);
	borderPoint->setPen(outlinePen);
	borderPoint->setBrush(blueBrush);
	border.push_back(borderPoint);
	scene->addItem(borderPoint);
}

void SpherePicking::updateBorderPoints() {
	for(size_t i = 0; i < border.size(); i++) {
		sphere->border[i] = border[i]->pos();
	}
	fit();
}



void SpherePicking::showHighlight(size_t n) {
	highlight->setVisible(sphere->fitted);

	if(!sphere->fitted)
		return;

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

void SpherePicking::updateHighlightPosition() {
	highlight->setBrush(Qt::green);
	QPen pen = highlight->pen();
	pen.setColor(Qt::green);
	highlight->setPen(pen);
	sphere->lights[this->currentImage()] = highlight->pos();
}

void SpherePicking::deleteSelected(int currentImage) {
	size_t j = 0;
	for(size_t i = 0; i < border.size(); i++, j++) {
		if(i != j) {
			border[j] = border[i];
			sphere->border[j] = sphere->border[i];
		}
		if(border[i]->isSelected()) {
			scene->removeItem(border[i]);
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

void SpherePicking::keyReleaseEvent(QKeyEvent *e) {
	switch(e->key()) {
	case Qt::Key_Backspace:
	case Qt::Key_Delete:
		deleteSelected(this->currentImage());
		return;
	case Qt::Key_Z:
		if((e->modifiers() | Qt::ControlModifier) && border.size()) {
			scene->removeItem(border.back());
			delete border.back();
			border.pop_back();
			sphere->border.pop_back();
			fit();
			return;
		}
		break;
	}
	ImageFrame::keyPressEvent(e);
}
