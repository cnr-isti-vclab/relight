#include <QFileDialog>
#include <QMessageBox>

#include "rtiexport.h"
#include "ui_rtiexport.h"

RtiExport::RtiExport(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::RtiExport) {
	ui->setupUi(this);

	connect(ui->basis, SIGNAL(currentIndexChanged(int)), this, SLOT(changeBasis(int)));
	connect(ui->planes, SIGNAL(valueChanged(int)), this, SLOT(changePlanes(int)));
	connect(this, SIGNAL(accepted()), this, SLOT(createRTI()));

}

RtiExport::~RtiExport()
{
	delete ui;
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


void RtiExport::createRTI() {
	QString output = QFileDialog::getSaveFileName(this, "Select an output directory");
	if(output.isNull()) return;

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
	builder.imageset.images = images;
	builder.imageset.lights = lights;
	builder.imageset.initImages(path.toStdString().c_str());

	builder.width  = builder.imageset.width;
	builder.height = builder.imageset.height;
	builder.lights = lights;

	if(!builder.init()) {
		QMessageBox::critical(this, "We have a problem!", QString(builder.error.c_str()));
		return;
	}
	builder.save(output.toStdString(), ui->quality->value());
}

