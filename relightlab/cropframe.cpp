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

	QGroupBox *bounds = new QGroupBox("Bounds");
	bounds->setMinimumWidth(300);
	right_side->addWidget(bounds, 1);

	QGridLayout * bounds_layout = new QGridLayout(bounds);

	bounds_layout->addWidget(new QLabel("Width"), 0, 0);
	bounds_layout->addWidget(crop_width = new QSpinBox, 0, 1);

	bounds_layout->addWidget(new QLabel("Height"), 1, 0);
	bounds_layout->addWidget(crop_height = new QSpinBox, 1, 1);

	bounds_layout->addWidget(new QLabel("Top"), 2, 0);
	bounds_layout->addWidget(crop_top = new QSpinBox, 2, 1);

	bounds_layout->addWidget(new QLabel("Left"), 3, 0);
	bounds_layout->addWidget(crop_left = new QSpinBox, 3, 1);

	bounds_layout->addWidget(new QLabel("Bottom"), 4, 0);
	bounds_layout->addWidget(crop_bottom = new QSpinBox, 4, 1);

	bounds_layout->addWidget(new QLabel("Right"), 5, 0);
	bounds_layout->addWidget(crop_right = new QSpinBox, 5, 1);

	crop_width->setMaximum(65535);
	crop_height->setMaximum(65535);
	crop_top->setMaximum(65535);
	crop_left->setMaximum(65535);
	crop_bottom->setMaximum(65535);
	crop_right->setMaximum(65535);



	QGroupBox *aspect_box = new QGroupBox("Aspect ratio");
	right_side->addWidget(aspect_box);

	QGridLayout *aspect_layout = new QGridLayout(aspect_box);

	QComboBox *aspect_combo = new QComboBox;
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

	right_side->addStretch(2);

	connect(aspect_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(setAspectRatio(int)));
	connect(cropper, SIGNAL(areaChanged(QRect)), this, SLOT(setArea(QRect)));

	//TODO needs to enforce aspect ratio!
	connect(crop_width, QOverload<int>::of(&QSpinBox::valueChanged), [&](int a) {
		QRect rect = cropper->croppedRect();
		rect.setWidth(a);
		cropper->setCrop(rect, true);
	});
	connect(crop_height, QOverload<int>::of(&QSpinBox::valueChanged), [&](int a) {
		QRect rect = cropper->croppedRect();
		rect.setHeight(a);
		cropper->setCrop(rect, true);
	});
	connect(crop_top, QOverload<int>::of(&QSpinBox::valueChanged), [&](int a) {
		QRect rect = cropper->croppedRect();
		rect.setTop(a);
		cropper->setCrop(rect, false);
	});
	connect(crop_left, QOverload<int>::of(&QSpinBox::valueChanged), [&](int a) {
		QRect rect = cropper->croppedRect();
		rect.setLeft(a);
		cropper->setCrop(rect, false);
	});
	connect(crop_bottom, QOverload<int>::of(&QSpinBox::valueChanged), [&](int a) {
		QRect rect = cropper->croppedRect();
		rect.setBottom(a);
		cropper->setCrop(rect, false);
	});
	connect(crop_right, QOverload<int>::of(&QSpinBox::valueChanged), [&](int a) {
		QRect rect = cropper->croppedRect();
		rect.setRight(a);
		cropper->setCrop(rect, false);
	});

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

void CropFrame::setAspectRatio(int aspect) {
	cropper->setProportionFixed(aspect > 0);

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
}

void CropFrame::setArea(QRect rect) {
	crop_width->setValue(rect.width());
	crop_height->setValue(rect.height());
	crop_top->setValue(rect.top());
	crop_left->setValue(rect.left());
	crop_bottom->setValue(rect.bottom());
	crop_right->setValue(rect.right());
}
