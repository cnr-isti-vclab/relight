#include <QVBoxLayout>
#include <QToolBar>
#include <QGraphicsView>
#include <QStatusBar>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QResizeEvent>
#include <QMessageBox>

#include "relightapp.h"
#include "imageframe.h"
//#include "canvas.h"
#include "imageview.h"
#include "imagelist.h"
#include "imagegrid.h"
#include "../src/sphere.h"

#include <iostream>
using namespace std;

ImageFrame::ImageFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *container = new QVBoxLayout(this);

	QHBoxLayout *toolbars = new QHBoxLayout();

	left_toolbar = new QToolBar;

	left_toolbar->addAction(qRelightApp->action("rotate_left"));
	left_toolbar->addAction(qRelightApp->action("rotate_right"));


	toolbars->addWidget(left_toolbar, 0, Qt::AlignLeft);

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
	connect(qRelightApp, SIGNAL(updateThumbnail(int)), image_grid, SLOT(updateThumbnail(int)));
	image_grid->hide();
	content->addWidget(image_grid, 1);

	content->addWidget(image_view = new ImageView, 1);

	connect(image_list, SIGNAL(skipChanged(int)), image_grid, SLOT(setSkipped(int)));
	connect(image_list, SIGNAL(skipChanged(int)), image_view, SLOT(setSkipped(int)));
	connect(image_list, SIGNAL(skipChanged(int)), this, SLOT(updateSkipped(int)));

	connect(image_grid, SIGNAL(skipChanged(int)), image_list, SLOT(setSkipped(int)));
	connect(image_grid, SIGNAL(skipChanged(int)), image_view, SLOT(setSkipped(int)));
	connect(image_grid, SIGNAL(skipChanged(int)), this, SLOT(updateSkipped(int)));

	connect(image_view, SIGNAL(skipChanged(int)), image_grid, SLOT(setSkipped(int)));
	connect(image_view, SIGNAL(skipChanged(int)), image_list, SLOT(setSkipped(int)));
	connect(image_view, SIGNAL(skipChanged(int)), this, SLOT(updateSkipped(int)));

	connect(qRelightApp->action("rotate_left"),  SIGNAL(triggered(bool)), this, SLOT(rotateLeft()));
	connect(qRelightApp->action("rotate_right"),  SIGNAL(triggered(bool)), this, SLOT(rotateRight()));

	connect(qRelightApp->action("zoom_fit"),  SIGNAL(triggered(bool)), image_view, SLOT(fit()));
	connect(qRelightApp->action("zoom_one"),  SIGNAL(triggered(bool)), image_view, SLOT(one()));
	connect(qRelightApp->action("zoom_in"),  SIGNAL(triggered(bool)), image_view, SLOT(zoomIn()));
	connect(qRelightApp->action("zoom_out"), SIGNAL(triggered(bool)), image_view, SLOT(zoomOut()));
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

void ImageFrame::clear() {
	image_list->clear();
	image_grid->clear();
	image_view->clear();
}

void ImageFrame::init() {
	image_list->init();
	image_grid->init();
	image_view->clear();

	if(qRelightApp->project().images.size()) {
		showImage(0);
		//resize scene rect, it does not get updated automatically
		image_view->setSceneRect(image_view->scene.itemsBoundingRect());
		image_list->setCurrentRow(0);
		image_view->fit();
	}
	listMode(); //TODO actually use last used mode used by the user but only in imageframe
}

void ImageFrame::updateSkipped(int n) {
//ask mr. project to update the sphere directions and dome lights if using spheres.
	Project &project = qRelightApp->project();
	Image &img = project.images[n];
	if(img.skip) {
		for(Sphere *sphere: project.spheres) {

			sphere->lights[n] = QPointF(0, 0);
			sphere->directions[n] = Eigen::Vector3f(0, 0, 0);
		}
	} else {
		QImage image;
		image.load(img.filename, "JPG");
		if(image.isNull()) {
			QMessageBox::critical(this, "Could not find an image", "Could not load image: " + img.filename + "!");
			return;
		}
		for(Sphere *sphere: project.spheres) {
			sphere->findHighlight(image, n, img.skip);
			sphere->computeDirections(project.lens);
		}
	}
	if(project.dome.label.isEmpty()) {
		project.dome.fromSpheres(project.images, project.spheres, project.lens);
	}
	emit skipChanged();
}

int ImageFrame::currentImage() {
	//TODO not properly elegant....
	return image_list->currentRow();
}

void ImageFrame::rotateLeft() {
	QList<QListWidgetItem*> selectedItems = image_list->selectedItems();
	for(QListWidgetItem *item: selectedItems) {
		int id = item->data(Qt::UserRole).toInt();
		rotateImage(id, false);
	}
	showImage(image_list->currentRow());
}

void ImageFrame::rotateRight() {
	QList<QListWidgetItem*> selectedItems = image_list->selectedItems();
	for(QListWidgetItem *item: selectedItems) {
		int id = item->data(Qt::UserRole).toInt();
		rotateImage(id, true);
	}
	showImage(image_list->currentRow());
}

void ImageFrame::rotateImage(int id, bool clockwise) {
	QTransform rotate;
	rotate.rotate(clockwise ? 90 : -90);

	Project &project = qRelightApp->project();

	assert(id >= 0 && id < project.images.size());

	Image &image = project.images[id];
	QImage source;
	source.load(image.filename, "JPG");
	QImage rotated = source.transformed(rotate);
	rotated.save(image.filename, "jpg", 100);
	//thumbs needs to be rotated!!!
	qRelightApp->loadThumbnails();
}

void ImageFrame::showImage(int id) {

	Project &project = qRelightApp->project();

	//image_list->setCurrentRow(id);
	qRelightApp->action("previous_image")->setEnabled(id != 0);
	qRelightApp->action("next_image")->setEnabled(id != (int)project.images.size()-1);

	image_view->showImage(id);

	int w = project.imgsize.width();
	int h = project.imgsize.height();
	status->showMessage(QString("%1x%2 %3").arg(w).arg(h).arg(QFileInfo(project.images[id].filename).canonicalFilePath()));
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
	image_view->show();
	image_list->hide();
	image_grid->hide();
}
void ImageFrame::listMode() {
	image_view->show();
	image_list->show();
	image_grid->hide();

}
void ImageFrame::gridMode() {
	image_view->hide();
	image_list->hide();
	image_grid->show();
}

