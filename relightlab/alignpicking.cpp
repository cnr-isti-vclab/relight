#include "alignpicking.h"
#include "canvas.h"
#include "../src/align.h"
#include "relightapp.h"

#include <QGraphicsRectItem>
#include <QPen>
#include <QKeyEvent>

AlignPicking::AlignPicking(QWidget *parent): ImageViewer(parent) {

	marker_side = 40;

	connect(view, SIGNAL(clicked(QPoint)), this, SLOT(click(QPoint)));
}


void AlignPicking::clear() {
	scene().clear();
	rect = nullptr;
}

void AlignPicking::setAlign(Align *a) {
	clear();
	align = a;

	rect = scene().addRect(a->rect, QPen(Qt::yellow), Qt::red);

	showImage(0);
	fit();
}



void AlignPicking::click(QPoint p) {
	clear();

	QSize imgsize = qRelightApp->project().imgsize;
	QPointF pos = view->mapToScene(p);

//ensure that the marker is inside the image
	pos.setX(std::max(marker_side/2.0, pos.x()));
	pos.setY(std::max(marker_side/2.0, pos.y()));


	pos.setX(std::min(imgsize.width()-marker_side/2.0, pos.x()));
	pos.setY(std::min(imgsize.height()-marker_side/2.0, pos.y()));

	align->rect = QRect(pos.x()-marker_side/2.0, pos.y()-marker_side/2.0, marker_side, marker_side);
	rect = scene().addRect(align->rect, QPen(Qt::yellow), Qt::red);
}
