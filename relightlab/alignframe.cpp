#include "alignframe.h"
#include "imageview.h"
#include "flowlayout.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsRectItem>

AlignFrame::AlignFrame(QWidget *parent): QFrame(parent) {
    //TODO: make a function for this to use in all frames. (inheritance?)
    setFrameStyle(QFrame::NoFrame);

    QVBoxLayout *content = new QVBoxLayout(this);

	content->addWidget(image_viewer = new ImageViewer());

	//content->addWidget(new FlowLayout());

	connect(image_viewer->view, SIGNAL(clicked(QPoint)), this, SLOT(click(QPoint)));
}

void AlignFrame::click(QPoint p) {
	QPointF pos = image_viewer->view->mapToScene(p);

	//min distance between border points in pixels.
	double minBorderDist = 20;
	if( sqrt(pow(p.x() - pos.x(), 2) + pow(p.y() - pos.y(), 2)) < minBorderDist) {
			return;
	}
	float side = 32;
	QGraphicsRectItem *rect = new QGraphicsRectItem(pos.x() - side, pos.y() - side, side*2, side*2);
	image_viewer->scene().addItem(rect);
}
