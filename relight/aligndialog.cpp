#include "aligndialog.h"
#include "ui_aligndialog.h"

#include "../src/project.h"
#include "qalignmarker.h"
#include "../src/align.h"

#include <QDebug>
#include <QLabel>
#include <QGraphicsPixmapItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsView>
#include <QGraphicsScene>


AlignDialog::AlignDialog(AlignMarker *a, Project *p, QWidget *parent) :
	QDialog(parent), align(a), project(p),	ui(new Ui::AlignDialog) {

	ui->setupUi(this);
	scene = new QGraphicsScene();

	for(auto image: project->images) {

		if(image.skip) continue;
		QImage img(project->dir.filePath(image.filename));
		if(img.isNull()) {
			close();
		}
		if(img.size() != project->imgsize) {
			close();
		}
		QRect rect = align->align->rect;
		rect = rect.intersected(QRect(QPoint(0, 0), img.size()));
		QImage cropped = img.copy(rect);
		QPixmap pix;
		pix.convertFromImage(cropped);

		QGraphicsPixmapItem *item = new QGraphicsPixmapItem(pix);
		item->setScale(scale);
		thumbs.push_back(item);
		scene->addItem(item);
	}
	QPen pen(Qt::green);
	pen.setWidth(1);
	for(auto offset: align->align->offsets) {
		QGraphicsEllipseItem *item = new QGraphicsEllipseItem(0, 0, 2, 2);
		item->setBrush(Qt::green);
		item->setPen(pen);
		offsets.push_back(item);
		scene->addItem(item);
	}

	ui->view->setScene(scene);
	resized();
}


void AlignDialog::resized() {
	if(thumbs.size() == 0)
		return;

	QSize size = align->align->rect.size()*scale;
	int cx = size.width()/2;
	int cy = size.height()/2;
	int sx = size.width() + margin;
	int sy = (size.height() + margin);

	int cols = ui->view->width() / sx;
	if(cols == 0) cols  = 1;
	int rows = 1 + (thumbs.size() -1) / cols;
	for(size_t i = 0; i < thumbs.size(); i++) {
		int row = i /cols;
		int col = i - row * cols;
		int x = margin + sx *col;
		int y = margin + sy * row;
		thumbs[i]->setPos(x, y);
		offsets[i]->setPos(x + cx +1, y + cy + 1);
	}
	scene->setSceneRect(0, 0, cols *sx, rows *sy );
}

void AlignDialog::resizeEvent(QResizeEvent */*e*/) {
	resized();
}


AlignDialog::~AlignDialog() {
	delete ui;
}
