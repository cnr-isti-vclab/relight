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
#include <QDebug>

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
	right_side->addWidget(bounds, 1);

	QGridLayout * area_layout = new QGridLayout(bounds);

	area_layout->addWidget(new QLabel("Width"), 0, 0);
	area_layout->addWidget(crop_width = new QSpinBox, 0, 1);

	area_layout->addWidget(new QLabel("Height"), 1, 0);
	area_layout->addWidget(crop_height = new QSpinBox, 1, 1);

	area_layout->addWidget(new QLabel("Top"), 2, 0);
	area_layout->addWidget(crop_top = new QSpinBox, 2, 1);

	area_layout->addWidget(new QLabel("Left"), 3, 0);
	area_layout->addWidget(crop_left = new QSpinBox, 3, 1);

	crop_width->setMaximum(65535);
	crop_height->setMaximum(65535);
	crop_top->setMaximum(65535);
	crop_left->setMaximum(65535);

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

	aspect_combo = new QComboBox;
	aspect_combo->addItem("None");      //0
	aspect_combo->addItem("Custom");    //1
	aspect_combo->addItem("Square");    //2
	aspect_combo->addItem("4:3 Photo"); //3
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
	connect(cropper, SIGNAL(areaChanged(QRect)), this, SLOT(setArea(QRect)));

	connect(crop_width, SIGNAL(valueChanged(int)), cropper, SLOT(setWidth(int)));
	connect(crop_height, SIGNAL(valueChanged(int)), cropper, SLOT(setHeight(int)));
	connect(crop_top, SIGNAL(valueChanged(int)), cropper, SLOT(setTop(int)));
	connect(crop_left, SIGNAL(valueChanged(int)), cropper, SLOT(setLeft(int)));

	connect(maximize, SIGNAL(clicked()), cropper, SLOT(maximizeCrop()));	
	connect(center, SIGNAL(clicked()), cropper, SLOT(centerCrop()));
}

void CropFrame::init() {
	Project &project = qRelightApp->project();

	QString filename = project.images[0].filename;

	QImage img(project.dir.filePath(filename));
	if(img.isNull()) {
		QMessageBox::critical(this, "Houston we have a problem!", "Could not load image " + filename);
		return;
	}
	cropper->setImage(QPixmap::fromImage(img));
}

void CropFrame::setAspectRatio() {
	int aspect = aspect_combo->currentIndex();
	double aspects[9][2] = { {1, 1}, {1, 1}, {1, 1}, {4, 3} , {3, 2}, {16, 10}, {16, 9}, {2, 3}, {3, 4} };

	switch(aspect) {
	case 0: return; //none
	case 1:
		aspects[1][0] = aspect_width->value();
		aspects[1][1] = aspect_height->value();
		break;
	}

	double *s = aspects[aspect];
	cropper->setProportion(QSizeF(s[0], s[1]));
	cropper->setProportionFixed(aspect > 0);
}

void CropFrame::setArea(QRect rect) {
	crop_width->setValue(rect.width());
	crop_height->setValue(rect.height());
	crop_top->setValue(rect.top());
	crop_left->setValue(rect.left());
}

