#include <QVBoxLayout>
#include <QToolBar>
#include <QGraphicsView>
#include <QStatusBar>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QResizeEvent>
#include <QDebug>

#include "relightapp.h"
#include "imageframe.h"
#include "canvas.h"
#include "imagelist.h"
#include "imagegrid.h"

#include <iostream>
using namespace std;

ImageFrame::ImageFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *container = new QVBoxLayout(this);

	QHBoxLayout *toolbars = new QHBoxLayout();

	left_toolbar = new QToolBar;

	left_toolbar->addAction(qRelightApp->action("rotate_left"));
	left_toolbar->addAction(qRelightApp->action("rotate_right"));


	toolbars->addWidget(left_toolbar, 0, Qt::AlignLeft);
//	toolbars->addSpacing();

	center_toolbar = new QToolBar;
	center_toolbar->addAction(qRelightApp->action("zoom_fit"));
	center_toolbar->addAction(qRelightApp->action("zoom_one"));
	center_toolbar->addAction(qRelightApp->action("zoom_in"));
	center_toolbar->addAction(qRelightApp->action("zoom_out"));
	center_toolbar->addAction(qRelightApp->action("previous_image"));
	center_toolbar->addAction(qRelightApp->action("next_image"));
	toolbars->addWidget(center_toolbar, 0, Qt::AlignCenter);


	right_toolbar = new QToolBar;
	right_toolbar->addAction(qRelightApp->action("show_image"));
	right_toolbar->addAction(qRelightApp->action("show_list"));
	right_toolbar->addAction(qRelightApp->action("show_grid"));
	toolbars->addWidget(right_toolbar, 0, Qt::AlignRight);

	container->addLayout(toolbars);

	QHBoxLayout *content = new QHBoxLayout;

	image_list = new ImageList();
	image_list->hide();
	content->addWidget(image_list, 0);

	image_grid = new ImageGrid();
	image_grid->hide();
	content->addWidget(image_grid, 1);

	canvas = new Canvas();
	content->addWidget(canvas, 1);

	canvas->setScene(&scene);
	//TODO: deal with double click on markers.
	//connect(canvas, SIGNAL(dblClicked(QPoint)), this, SLOT(doubleClick(QPoint)));
	//connect(canvas, SIGNAL(clicked(QPoint)), this, SLOT(pointClick(QPoint)));
	connect(qRelightApp->action("zoom_fit"),  SIGNAL(triggered(bool)), this, SLOT(fit()));
	connect(qRelightApp->action("zoom_one"),  SIGNAL(triggered(bool)), this, SLOT(one()));
	connect(qRelightApp->action("zoom_in"),  SIGNAL(triggered(bool)), canvas, SLOT(zoomIn()));
	connect(qRelightApp->action("zoom_out"), SIGNAL(triggered(bool)), canvas, SLOT(zoomOut()));
	connect(qRelightApp->action("previous_image"),  SIGNAL(triggered(bool)), this, SLOT(previousImage()));
	connect(qRelightApp->action("next_image"),  SIGNAL(triggered(bool)), this, SLOT(nextImage()));

	connect(image_list, SIGNAL(itemPressed(QListWidgetItem*)), this, SLOT(showImageItem(QListWidgetItem*)));

	connect(qRelightApp->action("show_image"),  SIGNAL(triggered(bool)), this, SLOT(imageMode()));
	connect(qRelightApp->action("show_list"),  SIGNAL(triggered(bool)), this, SLOT(listMode()));
	connect(qRelightApp->action("show_grid"),  SIGNAL(triggered(bool)), this, SLOT(gridMode()));

	container->addLayout(content);

	status = new QStatusBar();
	container->addWidget(status);
}


void ImageFrame::init() {
	image_list->init();
	image_grid->init();

	if(imagePixmap) {
		scene.removeItem(imagePixmap);
		imagePixmap = nullptr;
	}

	if(qRelightApp->project().images.size()) {
		showImage(0);
		fit();
	}
	listMode(); //TODO actually use last used mode used by the user but only in imageframe
}

int ImageFrame::currentImage() {
	//TODO not properly elegant....
	return image_list->currentRow();
}

void ImageFrame::showImage(int id) {
	Project &project = qRelightApp->project();

	image_list->setCurrentRow(id);
	qRelightApp->action("previous_image")->setEnabled(id != 0);
	qRelightApp->action("next_image")->setEnabled(id != (int)project.images.size()-1);

	QString filename = project.images[id].filename;

	QImage img(project.dir.filePath(filename));
	if(img.isNull()) {
		QMessageBox::critical(this, "Houston we have a problem!", "Could not load image " + filename);
		return;
	}
	imagePixmap = new QGraphicsPixmapItem(QPixmap::fromImage(img));
	imagePixmap->setZValue(-1);
	scene.addItem(imagePixmap);

	int w = project.imgsize.width();
	int h = project.imgsize.height();
	double sx =  double(canvas->width()) / w;
	double sy = double(canvas->height()) / h;
	double min_scale = std::min(1.0, std::min(sx, sy));
	canvas->min_scale = min_scale;
	status->showMessage(QString("%1x%2 %3").arg(w).arg(h).arg(QFileInfo(project.images[id].filename).canonicalFilePath()));
}

void ImageFrame::fit() {
	Project &project = qRelightApp->project();
	qDebug() << "Canvas: " << canvas->size() << " imgsize: " << project.imgsize;
	//find smallest problems
	double sx =  double(canvas->width()) / project.imgsize.width();
	double sy = double(canvas->height()) / project.imgsize.height();
	double s = std::min(sx, sy);
	double current_scale = canvas->transform().m11();
	s = s/current_scale;
	canvas->scale(s, s);

}

void ImageFrame::one() {
	double current_scale = canvas->transform().m11();
	double s = 1/current_scale;
	canvas->scale(s, s);
}


void ImageFrame::previousImage() {
	int current = image_list->currentRow();
	if(current-- <= 0)
		return;
	image_list->setCurrentRow(current);
	showImage(current);
}

void ImageFrame::nextImage() {
	int current = image_list->currentRow();
	if(current++ == (int)qRelightApp->project().images.size()-1)
		return;
	image_list->setCurrentRow(current);
	showImage(current);
	//TODO enable and disable previous//next maybe in showImage
}

void ImageFrame::showImageItem(QListWidgetItem *item) {
	int id =item->listWidget()->currentRow();
	showImage(id);
}

void ImageFrame::imageMode() {
	canvas->show();
	image_list->hide();
	image_grid->hide();
}
void ImageFrame::listMode() {
	canvas->show();
	image_list->show();
	image_grid->hide();

}
void ImageFrame::gridMode() {
	canvas->hide();
	image_list->hide();
	image_grid->show();
}

