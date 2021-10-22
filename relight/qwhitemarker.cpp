#include "qwhitemarker.h"
#include "white.h"
#include <QGraphicsRectItem>
#include <QPen>
#include <QLabel>
#include <QGraphicsScene>

QWhiteMarker::QWhiteMarker( White *m, QGraphicsView *_view, QWidget *parent):
	QMarker(_view, parent), white(m) {

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

QWhiteMarker::~QWhiteMarker() {
	delete rect;
}

void QWhiteMarker::setSelected(bool value) {
	QPen pen = rect->pen();
	pen.setWidth(value? 2 : 1);
	rect->setPen(pen);

	QMarker::setSelected(value);
}


void QWhiteMarker::onEdit() {
	//setSelected(false);
}

void QWhiteMarker::click(QPointF pos) {
	rect->setRect(QRectF(-16, -16, 32, 32).translated(pos));
	rect->setVisible(true);
	white->rect = rect->rect();
	setEditing(false);
}
