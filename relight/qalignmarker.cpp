#include "qalignmarker.h"
#include "align.h"
#include <QGraphicsRectItem>
#include <QPen>
#include <QLabel>
#include <QGraphicsScene>

#include "aligndialog.h"

AlignMarker::AlignMarker( Align *m, QGraphicsView *_view, QWidget *parent):
	Marker(_view, parent), align(m) {

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

AlignMarker::~AlignMarker() {
	delete rect;
}

void AlignMarker::setSelected(bool value) {
	QPen pen = rect->pen();
	pen.setWidth(value? 2 : 1);
	rect->setPen(pen);

	Marker::setSelected(value);
}


void AlignMarker::onEdit() {
	//setSelected(false);
}

void AlignMarker::click(QPointF pos) {
	rect->setRect(QRectF(-32, -32, 64, 64).translated(pos));
	rect->setVisible(true);
	align->rect = rect->rect().toRect();
	setEditing(false);
	emit showTable(this);
}
