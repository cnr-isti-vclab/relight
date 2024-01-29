#include <QVBoxLayout>
#include <QToolBar>
#include <QGraphicsView>
#include <QStatusBar>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>

#include "relightapp.h"
#include "imageframe.h"
#include "canvas.h"

#include <iostream>
using namespace std;

ImageFrame::ImageFrame() {
	QVBoxLayout *container = new QVBoxLayout(this);

	QHBoxLayout *toolbars = new QHBoxLayout();

	QToolBar *left_toolbar = new QToolBar;
	left_toolbar->addAction(qRelightApp->action("zoom_fit"));
	left_toolbar->addAction(qRelightApp->action("zoom_one"));
	left_toolbar->addAction(qRelightApp->action("zoom_in"));
	left_toolbar->addAction(qRelightApp->action("zoom_out"));
	left_toolbar->addAction(qRelightApp->action("rotate_right"));
	left_toolbar->addAction(qRelightApp->action("rotate_left"));

	toolbars->addWidget(left_toolbar, 0, Qt::AlignLeft);
	//toolbars->addSpacing();

	QToolBar *right_toolbar = new QToolBar;
	right_toolbar->setStyleSheet("background:red;");
	right_toolbar->addAction(qRelightApp->action("show_image"));
	right_toolbar->addAction(qRelightApp->action("show_list"));
	right_toolbar->addAction(qRelightApp->action("show_grid"));
	toolbars->addWidget(right_toolbar, 1, Qt::AlignRight);

	container->addLayout(toolbars);

	canvas = new Canvas();
	container->addWidget(canvas);
	scene = new QGraphicsScene;
	canvas->setScene(scene);
	//TODO: deal with double click on markers.
	//connect(canvas, SIGNAL(dblClicked(QPoint)), this, SLOT(doubleClick(QPoint)));
	//connect(canvas, SIGNAL(clicked(QPoint)), this, SLOT(pointClick(QPoint)));
	connect(qRelightApp->action("zoom_fit"),  SIGNAL(triggered(bool)), this, SLOT(fit()));
	connect(qRelightApp->action("zoom_one"),  SIGNAL(triggered(bool)), this, SLOT(one()));
	connect(qRelightApp->action("zoom_in"),  SIGNAL(triggered(bool)), canvas, SLOT(zoomIn()));
	connect(qRelightApp->action("zoom_out"), SIGNAL(triggered(bool)), canvas, SLOT(zoomOut()));


	status = new QStatusBar();
	container->addWidget(status);
}

void ImageFrame::showImage(int id) {
	Project &project = qRelightApp->project();
	QString filename = project.images[id].filename;

	QImage img(project.dir.filePath(filename));
	if(img.isNull()) {
		QMessageBox::critical(this, "Houston we have a problem!", "Could not load image " + filename);
		return;
	}
	if(imagePixmap)
		delete imagePixmap;
	imagePixmap = new QGraphicsPixmapItem(QPixmap::fromImage(img));
	imagePixmap->setZValue(-1);
	scene->addItem(imagePixmap);
} //new project loaded.

void ImageFrame::fit() {
	Project &project = qRelightApp->project();
	//find smallest problems
	double sx =  double(canvas->width()) / project.imgsize.width();
	double sy = double(canvas->height()) / project.imgsize.height();
	double s = std::min(sx, sy);
	double current_scale = canvas->transform().dx();
	cout << "Current Scale: " << current_scale << ", s: " << s << endl;
	s = s/current_scale;
	canvas->scale(s, s);
}

void ImageFrame::one() {
	Project &project = qRelightApp->project();
	double sx =  double(canvas->width()) / project.imgsize.width();
	double sy = double(canvas->height()) / project.imgsize.height();
	double current_scale = canvas->transform().dx();
	double s = 1/current_scale;
	canvas->scale(s, s);
}
