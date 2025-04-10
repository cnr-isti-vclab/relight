#include "cropframe.h"
#include "../relight/imagecropper.h"
#include "relightapp.h"

#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGridLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QMessageBox>
#include <QStandardItemModel>

CropFrame::CropFrame(QWidget *parent): QFrame(parent) {

	QVBoxLayout *content = new QVBoxLayout(this);

	QHBoxLayout *image_layout = new QHBoxLayout;
	content->addLayout(image_layout);
	cropper = new ImageCropper;

	image_layout->addWidget(cropper, 4);

	QVBoxLayout *right_side = new QVBoxLayout;
	image_layout->addLayout(right_side);

	QGroupBox *bounds = new QGroupBox("Area");
	bounds->setMinimumWidth(300);
	right_side->addWidget(bounds, 0);

	QGridLayout * area_layout = new QGridLayout(bounds);
	area_layout->setSpacing(10);

	area_layout->addWidget(new QLabel("Crop units:"), 0, 0);
	units = new QComboBox;
	units->addItem("px");
	units->addItem("mm");

	connect(units, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), [this]() { scaleChanged(); });

	area_layout->addWidget(units, 0, 1);

	area_layout->addWidget(new QLabel("Width"), 1, 0);
	area_layout->addWidget(crop_width = new QDoubleSpinBox, 1, 1);

	area_layout->addWidget(new QLabel("Height"), 2, 0);
	area_layout->addWidget(crop_height = new QDoubleSpinBox, 2, 1);

	area_layout->addWidget(new QLabel("Top"), 3, 0);
	area_layout->addWidget(crop_top = new QDoubleSpinBox, 3, 1);

	area_layout->addWidget(new QLabel("Left"), 4, 0);
	area_layout->addWidget(crop_left = new QDoubleSpinBox, 4, 1);

	area_layout->addWidget(new QLabel("Angle"), 5, 0);
	area_layout->addWidget(crop_angle = new QDoubleSpinBox, 5, 1);


	crop_top   ->setMaximum(65535);
	crop_left  ->setMaximum(65535);
	crop_width ->setMaximum(65535);
	crop_height->setMaximum(65535);
	crop_angle->setMaximum(360);

	crop_top   ->setDecimals(0);
	crop_left  ->setDecimals(0);
	crop_width ->setDecimals(0);
	crop_height->setDecimals(0);
	crop_angle->setDecimals(1);


	right_side->addSpacing(10);
	QHBoxLayout *maximize_layout = new QHBoxLayout;
	right_side->addLayout(maximize_layout);
	maximize_layout->setSpacing(20);

	QPushButton *maximize = new QPushButton("Maximize");
	maximize->setStyleSheet("text-align: center;");
	maximize->setProperty("class", "large");
	maximize_layout->addWidget(maximize);

	QPushButton *center = new QPushButton("Center");
	center->setProperty("class", "large");
	center->setStyleSheet("text-align: center;");
	maximize_layout->addWidget(center);

	right_side->addSpacing(10);

	QGroupBox *aspect_box = new QGroupBox("Aspect ratio");
	right_side->addWidget(aspect_box);

	QGridLayout *aspect_layout = new QGridLayout(aspect_box);
	aspect_layout->setSpacing(10);

	aspect_combo = new QComboBox;
	aspect_combo->addItem("None");                  //0
	aspect_combo->addItem("Custom");                //1
	aspect_combo->addItem("Square");                //2
	aspect_combo->addItem("4:3 Photo");             //3
	aspect_combo->addItem("3:2 Postcard");          //4
	aspect_combo->addItem("16:10 Widescreen");      //5
	aspect_combo->addItem("16:9 Widescreen");       //6
	aspect_combo->addItem("2:3 Postcard portrait"); //7
	aspect_combo->addItem("3:4 Photo portrait");    //8

	aspect_layout->addWidget(aspect_combo, 0, 0, 1, 2);


	aspect_layout->addWidget(aspect_width = new QSpinBox, 1, 0);
	aspect_layout->addWidget(aspect_height = new QSpinBox, 1, 1);
	aspect_width->setRange(1, 65535);
	aspect_height->setRange(1, 65535);

	right_side->addStretch(2);

	connect(aspect_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(setAspectRatio()));
	connect(aspect_width, SIGNAL(valueChanged(int)), this, SLOT(setAspectRatio()));
	connect(aspect_height, SIGNAL(valueChanged(int)), this, SLOT(setAspectRatio()));

	connect(crop_top,    &QDoubleSpinBox::editingFinished, [this]() { cropper->setTop   (round(crop_top   ->value()/pixelSize)); });
	connect(crop_left,   &QDoubleSpinBox::editingFinished, [this]() { cropper->setLeft  (round(crop_left  ->value()/pixelSize)); });
	connect(crop_width,  &QDoubleSpinBox::editingFinished, [this]() { cropper->setWidth (round(crop_width ->value()/pixelSize)); });
	connect(crop_height, &QDoubleSpinBox::editingFinished, [this]() { cropper->setHeight(round(crop_height->value()/pixelSize)); });
	connect(crop_angle,  &QDoubleSpinBox::editingFinished, [this]() { cropper->setAngle(round(crop_angle->value())); });

	connect(maximize, SIGNAL(clicked()), cropper, SLOT(maximizeCrop()));
	connect(center, SIGNAL(clicked()), cropper, SLOT(centerCrop()));

	connect(cropper, SIGNAL(areaChanged(QRect)), this, SLOT(updateCrop(QRect)));
}

void CropFrame::updateCrop(QRect rect) {
	Project &project = qRelightApp->project();
	project.crop = rect;
	setCrop(rect, cropper->angle);
	emit cropChanged(rect, cropper->angle);
}
void CropFrame::clear() {
	cropper->setImage(QPixmap());
}

void CropFrame::init() {
	Project &project = qRelightApp->project();
	QString filename = project.images[0].filename;
	int count = 0;
	while(project.images[count].skip == true) {
		count++;
		if(size_t(count) >= project.images.size())
			break;
		filename = project.images[count].filename;
	}
	auto *model = qobject_cast<QStandardItemModel*>(units->model());
	auto *item = model->item(1);
	item->setEnabled(project.pixelSize != 0.0f);

	QImage img(project.dir.filePath(filename));
	if(img.isNull()) {
		QMessageBox::critical(this, "Houston we have a problem!", "Could not load image " + filename);
		return;
	}
	cropper->setImage(QPixmap::fromImage(img));
	cropper->setCrop(project.crop);
}

void CropFrame::scaleChanged() {
	Project &project = qRelightApp->project();
	auto *model = qobject_cast<QStandardItemModel*>(units->model());
	auto *item = model->item(1);
	item->setEnabled(project.pixelSize != 0.0f);

	if(project.pixelSize == 0.0f &&  units->currentIndex() == 1) {
		units->setCurrentIndex(0);
	}
	setCrop(project.crop, project.crop_angle);
}

void CropFrame::setAspectRatio() {
	int aspect = aspect_combo->currentIndex();
	double aspects[9][2] = { {1, 1}, {1, 1}, {1, 1}, {4, 3} , {3, 2}, {16, 10}, {16, 9}, {2, 3}, {3, 4} };

	if(aspect == 1) { //custom
		aspects[aspect][0] = aspect_width->value();
		aspects[aspect][1] = aspect_height->value();
	}
	double *s = aspects[aspect];
	cropper->setProportionFixed(aspect > 0);
	if(aspect != 0)
		cropper->setProportion(QSizeF(s[0], s[1]));

}

void CropFrame::setCrop(QRect rect, float angle) {
	pixelSize = 1.0f;
	int d = 0;
	if(units->currentIndex() == 1)  {
		pixelSize = qRelightApp->project().pixelSize;
		d = 1;
	}
	crop_top   ->setDecimals(d);
	crop_left  ->setDecimals(d);
	crop_width ->setDecimals(d);
	crop_height->setDecimals(d);

	crop_top   ->setValue(rect.top()   *pixelSize);
	crop_left  ->setValue(rect.left()  *pixelSize);
	crop_width ->setValue(rect.width() *pixelSize);
	crop_height->setValue(rect.height()*pixelSize);
	crop_angle->setValue(angle);
}

