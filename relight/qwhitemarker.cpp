#include "qwhitemarker.h"
#include "../src/white.h"
#include <QGraphicsRectItem>
#include <QPen>
#include <QLabel>
#include <QGraphicsScene>

WhiteMarker::WhiteMarker( White *m, QGraphicsView *_view, QWidget *parent):
	Marker(_view, parent), white(m) {

	label->setText("White balance");

	rect = new QGraphicsRectItem();
	scene->addItem(rect);

	QPen pen;
	pen.setColor(Qt::yellow);
	pen.setWidth(1);
	rect->setPen(pen);
	rect->setBrush(QColor(255, 255, 0, 16));
	rect->setVisible(false);
}

WhiteMarker::~WhiteMarker() {
	delete rect;
}

void WhiteMarker::setSelected(bool value) {
	QPen pen = rect->pen();
	pen.setWidth(value? 2 : 1);
	rect->setPen(pen);

	Marker::setSelected(value);
}


void WhiteMarker::onEdit() {
	//setSelected(false);
}

void WhiteMarker::click(QPointF pos) {
	rect->setRect(QRectF(-16, -16, 32, 32).translated(pos));
	rect->setVisible(true);
	white->rect = rect->rect();
	setEditing(false);
}
