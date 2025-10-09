#include "cropframe.h"
#include "imagecropper.h"
#include "relightapp.h"
#include "helpbutton.h"

#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGridLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QMessageBox>
#include <QFileDialog>
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
	crop_angle->setMinimum(-360);

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

	right_side->addStretch(10);
	HelpedButton *crop_images = new HelpedButton("interface/crop", QIcon::fromTheme("crop"), "Crop images...");
	connect(crop_images, SIGNAL(clicked()), this, SLOT(cropImages()));
	right_side->addWidget(crop_images);

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

	connect(crop_top,    QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double v) { cropper->setTop   (round(v/pixelSize)); });
	connect(crop_left,   QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double v) { cropper->setLeft  (round(v/pixelSize)); });
	connect(crop_width,  QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double v) { cropper->setWidth (round(v/pixelSize)); });
	connect(crop_height, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double v) { cropper->setHeight(round(v/pixelSize)); });
	connect(crop_angle,  QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double v) { cropper->setAngle(v); });

	connect(maximize, SIGNAL(clicked()), cropper, SLOT(maximizeCrop()));
	connect(center, SIGNAL(clicked()), cropper, SLOT(centerCrop()));

	connect(cropper, SIGNAL(areaChanged(Crop)), this, SLOT(updateCrop(Crop)));
}

void CropFrame::updateCrop(Crop crop) {
	Project &project = qRelightApp->project();
	setCrop(crop);
	if(project.crop != crop) {
		project.crop = crop;
		emit cropChanged(cropper->crop); //crop might enforce boundaries.
	}
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

	cropper->showImage(0);
	cropper->setCrop(project.crop);
	cropper->fit();
}

void CropFrame::scaleChanged() {
	Project &project = qRelightApp->project();
	auto *model = qobject_cast<QStandardItemModel*>(units->model());
	auto *item = model->item(1);
	item->setEnabled(project.pixelSize != 0.0f);

	if(project.pixelSize == 0.0f &&  units->currentIndex() == 1) {
		units->setCurrentIndex(0);
	}
	setCrop(project.crop);
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

void CropFrame::setCrop(Crop crop) {
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

	crop_top   ->setValue(crop.top()   *pixelSize);
	crop_left  ->setValue(crop.left()  *pixelSize);
	crop_width ->setValue(crop.width() *pixelSize);
	crop_height->setValue(crop.height()*pixelSize);
	crop_angle->setValue(crop.angle);
}

void CropFrame::cropImages() {
	//select a few images
	QFileDialog dialog(this, "Select images to crop", qRelightApp->project().dir.absolutePath(), "Images (*.jpg *.jpeg *.png)");
	dialog.setFileMode(QFileDialog::ExistingFiles);
	dialog.setViewMode(QFileDialog::List);
	dialog.setOption(QFileDialog::DontUseNativeDialog, true);
	dialog.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
	dialog.setWindowTitle("Crop images");
	dialog.setLabelText(QFileDialog::Accept, "Crop");
	dialog.setLabelText(QFileDialog::Reject, "Cancel");
	if(!dialog.exec())
		return;
	QStringList files = dialog.selectedFiles();
	if(files.isEmpty()) {
		QMessageBox::warning(this, "No images selected", "Please select at least one image to crop.");
		return;
	}
	//select a folder
	QString folder = QFileDialog::getExistingDirectory(this, "Select folder to save cropped images", qRelightApp->project().dir.absolutePath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if(folder.isEmpty()) {
		QMessageBox::warning(this, "No folder selected", "Please select a folder to save cropped images.");
		return;
	}
	//get the folder of the selected images
	QString image_folder = QFileInfo(files[0]).absolutePath();
	//check the folder is not the same
	if(folder == image_folder) {
		QMessageBox::warning(this, "Same folder", "The selected folder is the same as the image folder. Please select a different folder to save cropped images.");
		return;
	}
	//ask for overwrite if cropped images exist
	QDir dir(folder);
	for(const QString &file : files) {
		QString filename = QFileInfo(file).fileName();
		if(dir.exists(filename)) {
			QMessageBox::StandardButton reply = QMessageBox::question(this, "Overwrite cropped images", 
				"The file " + filename + " already exists in the selected folder. Do you want to overwrite it?",
				QMessageBox::Yes | QMessageBox::No);
			if(reply == QMessageBox::No) {
				return; //cancel cropping
			}
		}
	}
	Crop &crop = qRelightApp->project().crop;
	//rotate and crop
	for(const QString &file : files) {
		QImage img(file);
		if(img.isNull()) {
			QMessageBox::critical(this, "Error", "Could not load image " + file + ". Skipping this image.");
			continue;
		}

		QImage cropped = img.copy(crop.boundingRect(img.size()));

		if(crop.angle != 0.0f) {
			cropped = crop.cropBoundingImage(cropped);
		}
		QString filename = QFileInfo(file).fileName();
		if(!cropped.save(dir.filePath(filename))) {
			QMessageBox::critical(this, "Error", "Could not save cropped image " + filename + ". Skipping this image.");
			continue;
		}
	}
}
