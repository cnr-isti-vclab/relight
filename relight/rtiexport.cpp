#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QGraphicsPixmapItem>
#include <QRect>
#include <QResizeEvent>
#include <QFuture>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrent>
#include <time.h>

#include "rtiexport.h"
#include "ui_rtiexport.h"
#include "imagecropper.h"
#include "../src/network/httpserver.h"
#include "scripts.h"
#include "processqueue.h"
#include "../src/rti.h"
#include "../src/cli/rtibuilder.h"

#include "normalstask.h"
#include "rtitask.h"

#include <functional>
#include <iostream>
#include "../src/jpeg_encoder.h"
using namespace std;

RtiExport::RtiExport(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RtiExport) {
    ui->setupUi(this);

    this->setWindowTitle("Relight- Export");

    ui->crop_frame->hide();
    connect(ui->basis, SIGNAL(currentIndexChanged(int)), this, SLOT(changeBasis(int)));
    connect(ui->planes, SIGNAL(valueChanged(int)), this, SLOT(changePlanes(int)));
    connect(ui->build_rti, SIGNAL(clicked()), this, SLOT(createRTI()));
    connect(ui->build_normals, SIGNAL(clicked()), this, SLOT(createNormals()));
//	connect(ui->close, SIGNAL(clicked()), this, SLOT(close()));
    connect(this, SIGNAL(rejected()), this, SLOT(close()));

    connect(ui->cropbuttonbox, SIGNAL(accepted()), this, SLOT(acceptCrop()));
    connect(ui->crop,          SIGNAL(clicked()),  this, SLOT(showCrop()));
    connect(ui->crop1,         SIGNAL(clicked()),  this, SLOT(showCrop()));

    connect(ui->cropreset, SIGNAL(clicked()), this, SLOT(resetCrop()));
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
    changeBasis(0);
}

void RtiExport::close() {
    server.stop();
    QDialog::close();
}

RtiExport::~RtiExport() {
    delete ui;
}

void RtiExport::setTabIndex(int index) {
    ui->export_frame->setCurrentIndex(index);
}

void RtiExport::setImages(QStringList _images) {
    images = _images;
}

void RtiExport::setCrop(QRect rect) {
    crop = rect;
    if(crop.isValid()) {
        ui->cropview->setCrop(rect);
        ui->cropview->showHandle();
    }
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
		ui->planes->setValue(9);
		break;
	case 1: //ptm
        ui->planes->setValue(18);
        break;
	case 2: //hsh 4
        ui->planes->setValue(12);
        break;
	case 3: //hsh 27
        ui->planes->setValue(27);
        break;
	case 4: //bilinear
        break;
	case 5: //rbf
        break;
	case 6: //yrbf
        break;
    }
	ui->chroma->setEnabled(n == 6 || n == 7);
	ui->planes->setEnabled(n >= 4);
}

void RtiExport::changePlanes(int n) {
    ui->chroma->setMaximum(n);
}

Rti::Type basis(int index) {
    //int b =  ui->basis->currentIndex();
	Rti::Type table[] =       { Rti::PTM, Rti::PTM, Rti::HSH, Rti::HSH, Rti::BILINEAR,  Rti::RBF, Rti::BILINEAR, Rti::RBF,  /* to do! */ Rti::DMD, Rti::SH, Rti::H };
    return table[index];
}

Rti::ColorSpace  colorSpace(int index) {
    //int b =  ui->basis->currentIndex();
	Rti::ColorSpace table[] = { Rti::LRGB, Rti::RGB, Rti::RGB, Rti::RGB, Rti::MRGB,     Rti::MRGB, Rti::MYCC, Rti::MYCC };
    return table[index];
}

void RtiExport::createNormals() {
    // Get export location
	QString output = QFileDialog::getSaveFileName(this, "Select an output file for normal:", "normals.png", "Images (*.png *.jpg)");
    if(output.isNull()) return;
	if(!output.endsWith(".png") && !output.endsWith(".jpg"))
		output += ".png";

    // Get normal method
    NormalSolver solver = NORMALS_L2; //least squares
    if(ui->l2_solver->isChecked())
        solver = NORMALS_L2;
    if(ui->sbl_solver->isChecked())
        solver = NORMALS_SBL;
    if(ui->rpca_solver->isChecked())
        solver = NORMALS_RPCA;


	FlatMethod flat_method = FlatMethod::NONE;
	if(ui->flat_radial->isChecked())
		flat_method = FlatMethod::RADIAL;
	if(ui->flat_fourier->isChecked())
		flat_method = FlatMethod::FOURIER;

	double flat_radius = ui->flat_fourier_radius->value();

	ProcessQueue &queue = ProcessQueue::instance();

	NormalsTask *task = new NormalsTask(project, solver, flat_method);
	task->input_folder = path;
	task->output = output;
	task->exportSurface = ui->export_surface->isChecked();
	task->exportDepthmap = ui->export_depthmap->isChecked();
	task->exportK = ui->discontinuity->value();
	task->flat_radius = ui->flat_fourier_radius->value();

	task->addParameter("images", Parameter::STRINGLIST, images);

	QList<QVariant> slights;
	for(auto light: lights)
		for(int k = 0; k < 3; k++)
			slights << QVariant(light[k]);
	task->addParameter("lights", Parameter::DOUBLELIST, slights);


	QRect rect = QRect(0, 0, 0, 0);
	if(ui->cropview->handleShown()) {
		rect = ui->cropview->croppedRect();
		task->addParameter("crop", Parameter::RECT,  rect);
	}

	if( pixelSize > 0 ) task->pixelSize = pixelSize;
	task->exportSurface = ui->export_surface->isChecked();
	task->exportK = ui->discontinuity->value();

	queue.addTask(task);
    queue.start();

    close();
}


void RtiExport::createRTI() {


	Rti::Type type = basis(ui->basis->currentIndex());

	QString output;
	if(ui->formatRTI->isChecked()) {
		if(type == Rti::HSH) {
			output = QFileDialog::getSaveFileName(this, "Select a file name", QString(), tr("RTI file (*.rti)"));
			if(output.isNull()) return;

			if(!output.endsWith(".rti"))
			output += ".rti";
      
		} else if(type == Rti::PTM) {
			output = QFileDialog::getSaveFileName(this, "Select a file name", QString(), tr("PTM file (*.ptm)"));
			if(output.isNull()) return;

			if(!output.endsWith(".ptm"))
			output += ".ptm";
		}
	} else {
		output = QFileDialog::getSaveFileName(this, "Select an output folder", QString());
		if(output.isNull()) return;
	}

	QString format;
	if(ui->formatRTI->isChecked())
		format = "rti";
	else if(ui->formatRelight->isChecked())
		format = "relight";
	else if(ui->formatDeepzoom->isChecked())
		format = "deepzoom";
	else if(ui->formatTarzoom->isChecked())
		format = "tarzoom";
	else if(ui->formatItarzoom->isChecked())
		format = "itarzoom";

	RtiTask *task = new RtiTask(*project);
    task->input_folder = path;
	task->output = output;
    task->label = "RTI"; //should use

	uint32_t ram = uint32_t(ui->ram->value());
	task->addParameter("ram", Parameter::INT, ram);
	task->addParameter("path", Parameter::FOLDER, path);
	task->addParameter("output", Parameter::FOLDER, output);
	task->addParameter("images", Parameter::STRINGLIST, images);
	task->addParameter("pixelSize", Parameter::DOUBLE, pixelSize);
	task->addParameter("light3d", Parameter::BOOL, light3d);


	QList<QVariant> slights;
	for(auto light: lights)
		for(int k = 0; k < 3; k++)
			slights << QVariant(light[k]);
	task->addParameter("lights", Parameter::DOUBLELIST, slights);

	task->addParameter("type", Parameter::INT,  type);
	task->addParameter("colorspace", Parameter::INT, colorSpace(ui->basis->currentIndex()));
	task->addParameter("nplanes", Parameter::INT, ui->planes->value());
	
	int chroma = ui->chroma->value();
	int nplanes = ui->planes->value();
	if(chroma*3 > nplanes) {
		QMessageBox::critical(this, "Invalid value", "Chroma planes cannot be larger than number of planes / 3");
		return;
	}
	int yplanes = nplanes - chroma*2;
	task->addParameter("yplanes", Parameter::INT, yplanes);

	QRect rect = QRect(0, 0, 0, 0);
	if(ui->cropview->handleShown()) {
		rect = ui->cropview->croppedRect();
		task->addParameter("crop", Parameter::RECT,  rect);
	}

	task->addParameter("quality", Parameter::INT, ui->quality->value());


	QStringList steps;
	
	if(format == "rti")
		steps << "rti";
	else {
		steps << "relight";
		if(format == "deepzoom")
			steps << "deepzoom";
		if(format == "tarzoom")
			steps << "deepzoom" << "tarzoom";
		if(format == "itarzoom")
			steps << "deepzoom" << "tarzoom" << "itarzoom";
		if(ui->openlime->isChecked())
			steps << "openlime";
	}

	task->addParameter("steps", Parameter::STRINGLIST, steps);

	ProcessQueue &queue = ProcessQueue::instance();
	queue.addTask(task);

	close();
}


void RtiExport::showCrop() {
    ui->cropview->showHandle();
    ui->export_frame->hide();
    ui->crop_frame->show();
}

void RtiExport::acceptCrop() {
    crop = ui->cropview->croppedRect();
    ui->crop_frame->hide();
    ui->export_frame->show();
}

void RtiExport::resetCrop() {
    ui->cropview->resetCrop();
    cropChanged(ui->cropview->croppedRect());
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
