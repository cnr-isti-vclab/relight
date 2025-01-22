#include "alignpicking.h"
#include "canvas.h"
#include "../src/align.h"
#include "relightapp.h"

#include <QGraphicsRectItem>
#include <QPen>
#include <QKeyEvent>


QVariant AlignRect::itemChange(GraphicsItemChange change, const QVariant &value)	{
	if ((change == ItemPositionChange  && scene()) || change == ItemScenePositionHasChanged) {
		picker->updateAlignPoint();
	}
	return QGraphicsItem::itemChange(change, value);
}

AlignPicking::AlignPicking(QWidget *parent): ImageViewer(parent) {

	marker_side = 40;
	connect(view, SIGNAL(clicked(QPoint)), this, SLOT(click(QPoint)));
	rect = new AlignRect(this, 0, 0, 0, 0);
	rect->setPen(QPen(Qt::yellow, 2));
	rect->setBrush(Qt::transparent);
}


void AlignPicking::clear() {
	if(rect) {
		scene().removeItem(rect);
	}
}

void AlignPicking::setAlign(Align *a) {
	clear();
	align = a;
	rect->setRect(align->rect);
	scene().addItem(rect);

	showImage(0);
	fit();
}



void AlignPicking::click(QPoint p) {
	//clear();

	QSize imgsize = qRelightApp->project().imgsize;
	QPointF pos = view->mapToScene(p);

//ensure that the marker is inside the image
	pos.setX(std::max(marker_side/2.0, pos.x()));
	pos.setY(std::max(marker_side/2.0, pos.y()));


	pos.setX(std::min(imgsize.width()-marker_side/2.0, pos.x()));
	pos.setY(std::min(imgsize.height()-marker_side/2.0, pos.y()));

	align->rect = QRect(pos.x()-marker_side/2.0, pos.y()-marker_side/2.0, marker_side, marker_side);
	rect->setRect(align->rect);
}

void AlignPicking::updateAlignPoint() {
	align->rect = rect->rect().toRect();
}

