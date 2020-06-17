#include "rtiexport.h"
#include "ui_rtiexport.h"

RtiExport::RtiExport(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::RtiExport) {
	ui->setupUi(this);

	connect(ui->basis, SIGNAL(currentIndexChanged(int)), this, SLOT(changeBasis(int)));
	connect(ui->planes, SIGNAL(currentIndexChanged(int)), this, SLOT(changePlanes(int)));
}

RtiExport::~RtiExport()
{
	delete ui;
}

void RtiExport::changeBasis(int n) {
	switch(n) {
	case 0: //ptm
		ui->planes->setValue(9);
		break;
	case 1: //hsh 12
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
