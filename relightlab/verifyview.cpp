#include "verifyview.h"

#include "reflectionview.h"
#include "relightapp.h"
#include "../src/sphere.h"

#include <QGraphicsPixmapItem>
#include <QGraphicsPathItem>
#include <QKeyEvent>
#include <QRectF>
#include <QDebug>
#include <assert.h>


VerifyMarker::VerifyMarker(VerifyView *_view, Marker _marker, QGraphicsItem *parent):
	QGraphicsItem(parent), view(_view), marker(_marker) {
	setCursor(Qt::CrossCursor);
	setFlag(QGraphicsItem::ItemIsMovable);
	setFlag(QGraphicsItem::ItemIsSelectable);
	setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
}

QPainterPath VerifyMarker::shape() const {
	QPainterPath path;
	if(marker == ALIGN)
		path.addRect(-3, -3, 6, 6);
	else
		path.addRect(-radius-2, -radius-2, 2*(radius+2), 2*(radius+2));
	return path;
}

QRectF VerifyMarker::boundingRect() const {
	return QRectF(-3, -3, 6, 6);
}

void VerifyMarker::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
	QPen pen(active ? Qt::green : Qt::red, marker == ALIGN? 0.2 : 2);
	painter->setPen(pen);
	if(marker == ALIGN) {
		painter->drawLine(QPointF(-2, 0), QPointF(2, 0));
		painter->drawLine(QPointF(0, -2), QPointF(0, 2));
	} else {
		painter->drawEllipse(QPointF(0, 0), radius, radius);
	}
}

QVariant VerifyMarker::itemChange(GraphicsItemChange change, const QVariant &value) {
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


VerifyView:: VerifyView(QImage &_image, int _height, QPointF &_pos, VerifyMarker::Marker _marker, QWidget *parent):
	QGraphicsView(parent), marker(_marker), image(_image), pos(_pos) {
	height = _height;
	setScene(&scene);

	setFocusPolicy(Qt::StrongFocus);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	QPixmap pix = QPixmap::fromImage(image);//.scaledToHeight(height);
	img_item = scene.addPixmap(pix);
	double scale = height/(double)pix.height();
	setFixedSize(pix.width()*scale, height);

	center = QPointF(image.width()/2.0f, image.height()/2.0f);

	marker_item = new VerifyMarker(this, marker);
	marker_item->radius = 8*scale;
	marker_item->setPos(pos + center);
	marker_item->active = !pos.isNull();
	scene.addItem(marker_item);
}


void VerifyView::update() {
	QPointF p = marker_item->pos();
	if(!img_item->boundingRect().contains(p)) {
		marker_item->setPos(center);
	}
	pos = p - center;
	marker_item->active = !pos.isNull();
}

void VerifyView::resizeEvent(QResizeEvent *) {
	fitInView(img_item->boundingRect()); //.sceneRect()); //img_item);
}

void VerifyView::keyPressEvent(QKeyEvent *event) {
	if (event->key() == Qt::Key_Delete) {
		marker_item->setPos(center);
		update();
		return;
	}
	QWidget::keyPressEvent(event);
}

