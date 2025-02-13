#include "alignpicking.h"
#include "canvas.h"
#include "../src/align.h"
#include "relightapp.h"

#include <QGraphicsRectItem>
#include <QPen>
#include <QKeyEvent>


QRectF AlignRect::boundingRect() const {
	return rect.adjusted(-2, -2, 2, 2);
}

QRect AlignRect::getRect() {

	QRectF r(rect.left() + pos().x(),
			 rect.top() + pos().y(),
			 rect.width(),
			 rect.height());

	//r.moveCenter(p); //unfortunately there is a -1 in the formula for legacy reasons.
	return r.toRect();
}

void AlignRect::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
	painter->setPen(QPen(Qt::yellow, 2));
	painter->setBrush(Qt::NoBrush);
	painter->drawRect(rect);

	painter->drawEllipse(QPoint(0, 0), 1, 1);
}

QVariant AlignRect::itemChange(GraphicsItemChange change, const QVariant &value)	{
	if ((change == ItemPositionChange  && scene()) || change == ItemScenePositionHasChanged) {
//		picker->updateAlignPoint();
	}
	return QGraphicsItem::itemChange(change, value);
}

void AlignRect::setRect(QRectF r) {
	prepareGeometryChange();  // Notify the scene about bounding rect change
	QRectF oldRect = boundingRect();
	rect = r;
	update(oldRect.united(boundingRect()));  // Repaint both old and new areas
}




AlignPicking::AlignPicking(QWidget *parent): ImageViewer(parent) {

	marker_side = 40;
	connect(view, SIGNAL(clicked(QPoint)), this, SLOT(click(QPoint)));
	rect = new AlignRect(this, marker_side);

	view->setCursor(Qt::CrossCursor);
}


void AlignPicking::clear() {
	if(rect) {
		scene().removeItem(rect);
	}
//	align = nullptr;
}

void AlignPicking::setAlign(QRectF align_rect) {
	clear();
//	align = a;
	rect->setPos(align_rect.center());
	rect->side = align_rect.width();

	scene().addItem(rect);

	showImage(0);
	fit();
}



void AlignPicking::click(QPoint p) {

	QSize imgsize = qRelightApp->project().imgsize;
	QPointF pos = view->mapToScene(p);

	//ensure that the marker is inside the image
	pos.setX(std::max(marker_side/2.0, pos.x()));
	pos.setY(std::max(marker_side/2.0, pos.y()));

	pos.setX(std::min(imgsize.width()-marker_side/2.0, pos.x()));
	pos.setY(std::min(imgsize.height()-marker_side/2.0, pos.y()));

	rect->setPos(pos);
	rect->update();
//	updateAlignPoint();
}

/*void AlignPicking::updateAlignPoint() {
	align->rect = rect->getRect();
}*/

void AlignPicking::keyPressEvent(QKeyEvent *event) {
	if(event->key() == Qt::Key_Plus)
		marker_side += 5;
	if(event->key() == Qt::Key_Minus)
		marker_side -= 5;
	QPointF center = rect->rect.center();

	QRectF r(center - QPointF(marker_side/2.0, marker_side/2.0),
			center + QPointF(marker_side/2.0, marker_side/2.0));
	rect->setRect(r);
	rect->update();
//	updateAlignPoint();
}
