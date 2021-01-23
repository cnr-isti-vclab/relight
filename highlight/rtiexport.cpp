#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QGraphicsPixmapItem>
#include <QRect>
#include <QResizeEvent>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>

#include "rtiexport.h"
#include "ui_rtiexport.h"
#include "imagecropper.h"

#include <functional>
#include <iostream>
using namespace std;

RtiExport::RtiExport(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::RtiExport) {
	ui->setupUi(this);
	
	ui->crop_frame->hide();
	connect(ui->basis, SIGNAL(currentIndexChanged(int)), this, SLOT(changeBasis(int)));
	connect(ui->planes, SIGNAL(valueChanged(int)), this, SLOT(changePlanes(int)));
	connect(this, SIGNAL(accepted()), this, SLOT(createRTI()));
	
	connect(ui->crop,          SIGNAL(clicked()),  this, SLOT(showCrop()));
	connect(ui->cropbuttonbox, SIGNAL(accepted()), this, SLOT(acceptCrop()));
	connect(ui->cropbuttonbox, SIGNAL(rejected()), this, SLOT(rejectCrop()));
	connect(ui->cropview, SIGNAL(areaChanged(QRect)), this, SLOT(cropChanged(QRect)));

	connect(ui->top, SIGNAL(valueChanged(int)), this, SLOT(updateCrop()));
	connect(ui->left, SIGNAL(valueChanged(int)), this, SLOT(updateCrop()));
	connect(ui->width, SIGNAL(valueChanged(int)), this, SLOT(updateCrop()));
	connect(ui->height, SIGNAL(valueChanged(int)), this, SLOT(updateCrop()));

	ui->aspect->addItem("None"); //0
	ui->aspect->addItem("Custom"); //1
	ui->aspect->addItem("Square"); //2
	ui->aspect->addItem("4:3 Photo"); //3
	ui->aspect->addItem("3:2 Postcard"); //4
	ui->aspect->addItem("16:10 Widescreen"); //5
	ui->aspect->addItem("16:9 Widescreen");    //6
	ui->aspect->addItem("2:3 Postcard portrait"); //7
	ui->aspect->addItem("3:4 Photo portrait"); //8
	connect(ui->aspect, SIGNAL(currentIndexChanged(int)), this, SLOT(setAspectRatio(int)));

	ui->cropview->hideHandle();
	ui->cropview->setBackgroundColor( Qt::lightGray );
	ui->cropview->setCroppingRectBorderColor( Qt::white);
}

RtiExport::~RtiExport() {
	delete ui;
}

void RtiExport::setImages(QStringList _images) {
	images = _images;
}

ostream& operator<<(ostream& os, const QRectF& r) {
	os << "top: " << r.top() << " left: " << r.left() << " width: " << r.width() << " height: " << r.height();
	return os;
}
ostream& operator<<(ostream& os, const QRect& r) {
	os << "top: " << r.top() << " left: " << r.left() << " width: " << r.width() << " height: " << r.height();
	return os;
}

void RtiExport::showImage(QPixmap pix) {
	ui->cropview->setImage(pix);	
}

void RtiExport::changeBasis(int n) {
	switch(n) {
	case 0: //ptm
		ui->planes->setValue(18);
		break;
	case 1: //hsh 4
		ui->planes->setValue(12);
		break;
	case 2: //hsh 27
		ui->planes->setValue(27);
		break;
	case 3: //bilinear
		break;
	case 4: //rbf
		break;
	case 5: //yrbf
		break;
	}
	ui->chroma->setEnabled(n == 5);
	ui->planes->setEnabled(n >= 3);
}

void RtiExport::changePlanes(int n) {
	ui->chroma->setMaximum(n);
}

Rti::Type RtiExport::basis() {
	int b =  ui->basis->currentIndex();
	Rti::Type table[] =       { Rti::PTM, Rti::HSH, Rti::HSH, Rti::BILINEAR,  Rti::RBF, Rti::BILINEAR, Rti::RBF };
	return table[b];
}

Rti::ColorSpace  RtiExport::colorSpace() {
	int b =  ui->basis->currentIndex();
	Rti::ColorSpace table[] = { Rti::RGB, Rti::RGB, Rti::RGB, Rti::MRGB,     Rti::MRGB, Rti::MYCC, Rti::MYCC };
	return table[b];
	
}


bool RtiExport::callback(std::string s, int n) {
	QString str(s.c_str());
	emit progressText(str);
	emit progress(n);

	if(cancel) {
		cancel = false;
		return false;
	}
	return true;
}

void RtiExport::makeRti(QString output, QRect rect) {
	
	try {
		RtiBuilder builder;
		
		builder.type         = basis();
		builder.colorspace   = colorSpace();
		builder.nplanes      = ui->planes->value();
		builder.yccplanes[0] = ui->chroma->value();
		//builder.sigma =
		
		if( builder.colorspace == Rti::MYCC) {
			builder.yccplanes[1] = builder.yccplanes[2] = (builder.nplanes - builder.yccplanes[0])/2;
			builder.nplanes = builder.yccplanes[0] + 2*builder.yccplanes[1];
		}
		builder.crop[0] = rect.left();
		builder.crop[1] = rect.top();
		builder.crop[2] =  rect.width();
		builder.crop[3] = rect.height();
		builder.imageset.images = images;
		builder.lights = builder.imageset.lights = lights;
		builder.imageset.initImages(path.toStdString().c_str());
		builder.imageset.crop(rect.left(), rect.top(), rect.width(), rect.height());
		
		builder.width  = builder.imageset.width;
		builder.height = builder.imageset.height;
		builder.savemeans = true;
		
		std::function<bool(std::string s, int n)> callback = [this](std::string s, int n)->bool { return this->callback(s, n); };
		
		if(!builder.init(&callback)) {
			QMessageBox::critical(this, "We have a problem!", QString(builder.error.c_str()));
			return;
		}
		builder.save(output.toStdString(), ui->quality->value());

	} catch(int status) {
		if(status == 1) { //was canceled.
			emit progressText("Canceling...");
			emit progress(0);
		}
	} catch(std::exception e) {
		QMessageBox::critical(this, "We have a problem 1!", e.what());
	} catch(const char *str) {
		QMessageBox::critical(this, "We have a problem 2 !",str);
	} catch(...) {
		cout << "Something went wrong!" << endl;
		QMessageBox::critical(this, "We have a problem 3!", "Unknown error!");
		
	}
}


void RtiExport::createRTI() {
	QString output = QFileDialog::getSaveFileName(this, "Select an output directory");
	if(output.isNull()) return;
	
	
	progressbar = new QProgressDialog("Building RTI...", "Cancel", 0, 100, this);
	progressbar->setAutoClose(false);
	progressbar->setAutoReset(false);
	progressbar->setWindowModality(Qt::WindowModal);
	progressbar->show();
	connect(progressbar, SIGNAL(canceled()), this, SLOT(cancelProcess()));
	
	QRect rect = QRect(0, 0, 0, 0);
	if(ui->cropview->handleShown()) {
		rect = ui->cropview->croppedRect();
		cout << "Cropping! " << rect << endl << flush;
	}
	
	
	QFuture<void> future = QtConcurrent::run([this, output, rect]() { this->makeRti(output, rect); } );
	watcher.setFuture(future);
	connect(&watcher, SIGNAL(finished()), this, SLOT(finishedProcess()));
	connect(this, SIGNAL(progress(int)), progressbar, SLOT(setValue(int)));
	connect(this, SIGNAL(progressText(const QString &)), progressbar, SLOT(setLabelText(const QString &)));
}

void RtiExport::cancelProcess() {
	cancel = true;
}

void RtiExport::finishedProcess() {
	if(progressbar == nullptr)
		return;
	progressbar->close();
	delete progressbar;
	progressbar = nullptr;
}

void RtiExport::showCrop() {
	ui->cropview->showHandle();
	ui->export_frame->hide();
	ui->crop_frame->show();
}

void RtiExport::acceptCrop() {
	ui->export_frame->show();
	ui->crop_frame->hide();
}
void RtiExport::rejectCrop() {
	ui->cropview->hideHandle();
	ui->export_frame->show();
	ui->crop_frame->hide();
}

void RtiExport::cropChanged(QRect rect) {
	
	ui->width->setValue(rect.width());
	ui->height->setValue(rect.height());
	ui->left->setValue(rect.left());
	ui->top->setValue(rect.top());
}

void RtiExport::updateCrop() {
	ui->cropview->setCrop(QRect(ui->left->value(), ui->top->value(),
								ui->width->value(), ui->height->value()));
}

void RtiExport::setAspectRatio(int aspect) {
	ui->cropview->setProportionFixed(aspect > 0);
	
	double aspects[9][2] = { {1, 1}, {1, 1}, {1, 1}, {4, 3} , {3, 2}, {16, 10}, {16, 9}, {2, 3}, {3, 4} };
	
	switch(aspect) {
	case 0: return; //none
	case 1:
		aspects[1][0] = ui->aspect_width->value();
		aspects[1][1] =  ui->aspect_height->value();
		break;
	}

	double *s = aspects[aspect];
	ui->cropview->setProportion(QSizeF(s[0], s[1]));
}
