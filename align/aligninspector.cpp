#include "aligninspector.h"
#include "ui_aligninspector.h"

#include <QGraphicsPixmapItem>

using namespace std;

AlignInspector::AlignInspector(std::vector<QPointF> _offsets, std::vector<QImage> _samples, size_t _reference, QWidget *parent):
	QDialog(parent),
	offsets(_offsets),
	samples(_samples),
	reference(_reference),
	ui(new Ui::AlignInspector) {

	ui->setupUi(this);
	item = new QGraphicsPixmapItem(QPixmap::fromImage(samples[0]));
	scene = new QGraphicsScene(this);
	ui->graphicsView->setScene(scene);

	scene->addItem(item);

	setImage(0);

	ui->slider->setMinimum(0);
	ui->slider->setMaximum(offsets.size()-1);
	ui->dx->setMinimum(-30);
	ui->dx->setMaximum(30);
	ui->dy->setMinimum(-30);
	ui->dy->setMaximum(30);

	connect(ui->slider, SIGNAL(valueChanged(int)), this, SLOT(setImage(int)));
	connect(ui->ok, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->previous, SIGNAL(clicked()), this, SLOT(previous()));
	connect(ui->next, SIGNAL(clicked()), this, SLOT(next()));

	connect(ui->up, SIGNAL(clicked()), this, SLOT(up()));
	connect(ui->down, SIGNAL(clicked()), this, SLOT(down()));
	connect(ui->right, SIGNAL(clicked()), this, SLOT(right()));
	connect(ui->left, SIGNAL(clicked()), this, SLOT(left()));

	connect(ui->dx, SIGNAL(valueChanged(int)), this, SLOT(setOffset()));
	connect(ui->dy, SIGNAL(valueChanged(int)), this, SLOT(setOffset()));

}

AlignInspector::~AlignInspector() {
	delete ui;
}


void AlignInspector::update() {
	QImage img = samples[0];//(samples[0].size(), QImage::Format_RGB32);
	int w = samples[0].width();
	int h = samples[0].height();
	QPointF o = offsets[current_image];
	ui->dx->setValue(o.x());
	ui->dy->setValue(o.y());

	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			if(x + o.x() < 0 || x + o.x() >= w) continue;
			if(y + o.y() < 0 || y + o.y() >= w) continue;

			QRgb a = samples[current_image].pixel(x + o.x(), y + o.y());
			QRgb b = samples[reference].pixel(x, y);
			QRgb c = qRgb(128 + (qRed(a) - qRed(b))/2, 128 + (qGreen(a) - qGreen(b))/2, 128 + (qBlue(a) - qBlue(b))/2);
			img.setPixel(x, y, c);
		}
	}

	item->setPixmap(QPixmap::fromImage(img));
	ui->graphicsView->fitInView(QRect(0, 0, samples[0].width(), samples[0].height()), Qt::KeepAspectRatio);

}

void AlignInspector::setImage(int id) {


	//enable and disable buttons
	ui->previous->setEnabled(id != 0);
	ui->next->setEnabled(id != samples.size()-1);

//	ui->dx->setValue(offsets[id].x());
// 	ui->dx->setValue(offsets[id].y());

	current_image = id;
	update();
}


void AlignInspector::previous() {
	if(current_image == 0) return;
	setImage(current_image-1);
}

void AlignInspector::next() {
	if(current_image == samples.size()-1)
		return;
	setImage(current_image+1);
}

void AlignInspector::up() {
	offsets[current_image].ry()--;
	update();
}

void AlignInspector::down() {
	offsets[current_image].ry()++;
	update();
}

void AlignInspector::right() {
	offsets[current_image].rx()--;
	update();
}

void AlignInspector::left() {
	offsets[current_image].rx()++;
	update();
}

void AlignInspector::setOffset() {
	return;
	offsets[current_image].rx() = ui->dx->value();
	offsets[current_image].ry() = ui->dy->value();
	update();
}

