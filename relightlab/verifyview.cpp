#include "verifyview.h"

#include "reflectionview.h"
#include "relightapp.h"
#include "../src/sphere.h"

#include <QGraphicsPixmapItem>
#include <QRectF>
#include <QDebug>
#include <assert.h>

ReflectionPoint::ReflectionPoint(VerifyView *_view, qreal x, qreal y, qreal w, qreal h, QGraphicsItem *parent):
	QGraphicsEllipseItem(x, y, w, h, parent), view(_view) {
	init();
}
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
		view->updateReflection();
	}
	return QGraphicsItem::itemChange(change, value);
}

VerifyView:: VerifyView(QImage &_image, QPointF &_pos, int _height, QWidget *parent):
	QGraphicsView(parent), image(_image), pos(_pos) {
	height = _height;
	setScene(&scene);

	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	QPixmap pix = QPixmap::fromImage(image);//.scaledToHeight(height);
	img_item = scene.addPixmap(pix);
	double scale = height/(double)pix.height();
	setFixedSize(pix.width()*scale, height);

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

void VerifyView::updateReflection() {
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


void VerifyView::resizeEvent(QResizeEvent *event) {
	fitInView(img_item->boundingRect()); //.sceneRect()); //img_item);
}


