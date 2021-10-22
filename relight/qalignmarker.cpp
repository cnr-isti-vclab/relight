#include "qalignmarker.h"
#include "align.h"
#include <QGraphicsRectItem>
#include <QPen>
#include <QLabel>
#include <QGraphicsScene>

QAlignMarker::QAlignMarker( Align *m, QGraphicsView *_view, QWidget *parent):
	QMarker(_view, parent), align(m) {

	label->setText("Alignment");

	rect = new QGraphicsRectItem();
	scene->addItem(rect);

	QPen pen;
	pen.setColor(Qt::yellow);
	pen.setWidth(1);
	rect->setPen(pen);
	rect->setBrush(QColor(255, 255, 0, 16));
	rect->setVisible(false);
}

QAlignMarker::~QAlignMarker() {
	delete rect;
}

void QAlignMarker::setSelected(bool value) {
	QPen pen = rect->pen();
	pen.setWidth(value? 2 : 1);
	rect->setPen(pen);

	QMarker::setSelected(value);
}


void QAlignMarker::onEdit() {
	//setSelected(false);
}

void QAlignMarker::click(QPointF pos) {
	rect->setRect(QRectF(-64, -64, 128, 128).translated(pos));
	rect->setVisible(true);
	align->rect = rect->rect();
	setEditing(false);
}
