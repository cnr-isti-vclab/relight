#include "imageview.h"
#include "relightapp.h"
#include "../src/project.h"

#include <QGraphicsPixmapItem>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QToolBar>


ImageView::ImageView(QWidget *parent): Canvas(parent) {
	setScene(&scene);
	
}

void ImageView::clear() {
	//remove all elements from scene.
	scene.clear();
	imagePixmap = nullptr;
}

void ImageView::showImage(int id) {
	Project &project = qRelightApp->project();
	if(project.images.size() <= size_t(id))
		return;

	QString filename = project.images[id].filename;

	QImage img(project.dir.filePath(filename));
	if(img.isNull()) {
		QMessageBox::critical(this, "Houston we have a problem!", "Could not load image " + filename);
		return;
	}
	if(imagePixmap) {
		delete imagePixmap;
	}
	imagePixmap = new QGraphicsPixmapItem(QPixmap::fromImage(img));
	imagePixmap->setZValue(-1);
	scene.addItem(imagePixmap);

	int w = project.imgsize.width();
	int h = project.imgsize.height();
	double sx =  double(width()) / w;
	double sy = double(height()) / h;
	double min_scale = std::min(1.0, std::min(sx, sy));
	min_scale = min_scale;

	current_image = id;
}

void ImageView::setSkipped(int image, bool skip) {
	if(image != current_image)
		return;
}



void ImageView::fit() {
	if(imagePixmap)
		fitInView(imagePixmap->boundingRect());
}

void ImageView::one() {
	double current_scale = transform().m11();
	double s = 1/current_scale;
	scale(s, s);
}

void ImageView::next() {
	if(size_t(current_image+1) < qRelightApp->project().images.size())
		showImage(current_image+1);
}

void ImageView::prev() {
	if(current_image > 0)
		showImage(current_image-1);
}


ImageViewer::ImageViewer(QWidget *parent): QFrame(parent) {
	QVBoxLayout *layout = new QVBoxLayout(this);
	
	layout->addWidget(toolbar = new QToolBar(), 0, Qt::AlignCenter);
	layout->addWidget(view = new ImageView());

	QAction *fit = qRelightApp->action("zoom_fit");
	toolbar->addAction(fit->icon(), fit->text(), view, SLOT(fit()));

	QAction *one = qRelightApp->action("zoom_one");
	toolbar->addAction(one->icon(), one->text(), view, SLOT(one()));

	QAction *in = qRelightApp->action("zoom_in");
	toolbar->addAction(in->icon(), in->text(), view, SLOT(zoomIn()));

	QAction *out = qRelightApp->action("zoom_out");
	toolbar->addAction(out->icon(), out->text(), view, SLOT(zoomOut()));

	QAction *prev = qRelightApp->action("previous_image");
	toolbar->addAction(prev->icon(), prev->text(), view, SLOT(prev()));

	QAction *next = qRelightApp->action("next_image");
	toolbar->addAction(next->icon(), next->text(), view, SLOT(next()));
}

void ImageViewer::showImage(int id) {
	view->showImage(id);
}

