#include "verifyview.h"

#include "reflectionview.h"
#include "relightapp.h"
#include "../src/sphere.h"

#include <QGraphicsPixmapItem>
#include <QGraphicsPathItem>
#include <QRectF>
#include <QDebug>
#include <assert.h>

ReflectionPoint::ReflectionPoint(VerifyView *_view, QRectF rect, QGraphicsItem *parent):
	QGraphicsEllipseItem(rect, parent), view(_view) {
	init();
}

void ReflectionPoint::init() {
	setCursor(Qt::CrossCursor);
	setFlag(QGraphicsItem::ItemIsMovable);
	setFlag(QGraphicsItem::ItemIsSelectable);
	setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
}


AlignPoint::AlignPoint(VerifyView *_view, QGraphicsItem *parent):
	QGraphicsPathItem(parent), view(_view) {
	QPainterPath path;
	path.moveTo(-2, 0);
	path.lineTo(2, 0);
	path.moveTo(0, -2);
	path.lineTo(0, 2);
	setPath(path);
	init();
}

void AlignPoint::init() {
	setCursor(Qt::CrossCursor);
	setFlag(QGraphicsItem::ItemIsMovable);
	setFlag(QGraphicsItem::ItemIsSelectable);
	setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
}

QVariant ReflectionPoint::itemChange(GraphicsItemChange change, const QVariant &value)	{
	if ((change == ItemPositionChange  && scene()) ) {

		QPointF newPos = value.toPointF();
		QRectF rect = scene()->sceneRect();
		if (!rect.contains(newPos)) {
			// Keep the item inside the scene rect.
			newPos.setX(qMin(rect.right(), qMax(newPos.x(), rect.left())));
			newPos.setY(qMin(rect.bottom(), qMax(newPos.y(), rect.top())));
		}
		return newPos;
	}

	if(change == ItemScenePositionHasChanged) {
		view->update();
	}
	return QGraphicsItem::itemChange(change, value);
}

QVariant AlignPoint::itemChange(GraphicsItemChange change, const QVariant &value)	{
	if ((change == ItemPositionChange  && scene()) ) {

		QPointF newPos = value.toPointF();
		QRectF rect = scene()->sceneRect();
		if (!rect.contains(newPos)) {
			// Keep the item inside the scene rect.
			newPos.setX(qMin(rect.right(), qMax(newPos.x(), rect.left())));
			newPos.setY(qMin(rect.bottom(), qMax(newPos.y(), rect.top())));
		}
		return newPos;
	}

	if(change == ItemScenePositionHasChanged) {
		view->update();
	}
	return QGraphicsItem::itemChange(change, value);
}

VerifyView:: VerifyView(QImage &_image, int _height, QPointF &_pos, QWidget *parent):
	QGraphicsView(parent), image(_image), pos(_pos) {
	height = _height;
	setScene(&scene);

	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	QPixmap pix = QPixmap::fromImage(image);//.scaledToHeight(height);
	img_item = scene.addPixmap(pix);
	double scale = height/(double)pix.height();
	setFixedSize(pix.width()*scale, height);
}

ReflectionVerify::ReflectionVerify(QImage&_image, int _height, QPointF &_pos, QWidget *parent):
	VerifyView(_image, _height, _pos, parent) {
	double scale = height/double(image.height());
	int r = 8*scale;
	reflection = new ReflectionPoint(this, QRectF(QPointF(-r, -r), QSize(2*r, 2*r)));
	if(pos.isNull()) {
		reflection->setPos(image.width()/2, image.height()/2);
		reflection->setPen(QPen(Qt::red, 2));

	} else {
		reflection->setPos(pos);
		reflection->setPen(QPen(Qt::green, 2));
	}
	scene.addItem(reflection);
}

AlignVerify::AlignVerify(QImage&_image, int _height, QPointF &_pos, QWidget *parent):
	VerifyView(_image, _height, _pos, parent) {

	align = new AlignPoint(this);
	if(pos.isNull()) {
		align->setPos(image.width()/2, image.height()/2);
		align->setPen(QPen(Qt::red, 0.2));

	} else {
		align->setPos(pos);
		align->setPen(QPen(Qt::green, 0.2));
	}
	scene.addItem(align);
}


void ReflectionVerify::update() {
	QPointF p = reflection->pos();
	if(!img_item->boundingRect().contains(p)) {
		reflection->setPos(image.width()/2, image.height()/2);
		reflection->setPen(QPen(Qt::red, 2));
		pos = QPointF(0, 0); //order is important: setPos triggers again.

	} else {
		pos = p;
		reflection->setPen(QPen(Qt::green, 2));
	}
}


void AlignVerify::update() {
	QPointF p = align->pos();
	if(!img_item->boundingRect().contains(p)) {
		align->setPos(image.width()/2, image.height()/2);
		align->setPen(QPen(Qt::red, 0.2));
		pos = QPointF(0, 0); //order is important: setPos triggers again.

	} else {
		pos = p;
		align->setPen(QPen(Qt::green, 0.2));
	}
}


void VerifyView::resizeEvent(QResizeEvent *) {
	fitInView(img_item->boundingRect()); //.sceneRect()); //img_item);
}


