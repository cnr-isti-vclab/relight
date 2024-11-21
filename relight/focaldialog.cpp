#include "focaldialog.h"
#include "ui_focaldialog.h"
#include "../src/project.h"

FocalDialog::FocalDialog(Project *_project, QWidget *parent) :
	QDialog(parent),
	project(_project),
	ui(new Ui::FocalDialog) {

	ui->setupUi(this);

	this->setWindowTitle("Relight - Lens parameters");

	Lens &lens = project->lens;

	ui->width->setValue(lens.width);
	ui->height->setValue(lens.height);



	ui->focalLength->setValue(lens.focalLength);
	ui->ccdWidth->setValue(lens.ccdWidth());
	ui->ccdHeight->setValue(lens.ccdHeight());
	ui->principalOffsetX->setValue(lens.principalOffsetX);
	ui->principalOffsetY->setValue(lens.principalOffsetY);
	ui->k1->setValue(lens.k1);
	ui->k2->setValue(lens.k2);
	ui->p1->setValue(lens.p1);
	ui->p2->setValue(lens.p2);

	ui->equivalent->setChecked(lens.focal35equivalent);
	ui->real->setChecked(!lens.focal35equivalent);

	ui->ccdWidth->setDisabled(lens.focal35equivalent);
	ui->ccdHeight->setDisabled(lens.focal35equivalent);

	connect(ui->equivalent, SIGNAL(clicked(bool)), this, SLOT(setAsEquivalent()));
	connect(ui->real,       SIGNAL(clicked(bool)), this, SLOT(setAsReal()));
}

FocalDialog::~FocalDialog() {
	delete ui;
}
void FocalDialog::accept() {
	Lens &lens = project->lens;
	lens.focalLength = ui->focalLength->value();
	lens.pixelSizeX = ui->ccdWidth->value()/lens.width;
	lens.pixelSizeY = ui->ccdHeight->value()/lens.height;
	lens.principalOffsetX = ui->principalOffsetX->value();
	lens.principalOffsetY = ui->principalOffsetY->value();
	lens.k1 = ui->k1->value();
	lens.k1 = ui->k2->value();
	lens.p1 = ui->p1->value();
	lens.p1 = ui->p1->value();
}

void FocalDialog::setAsReal() {
	project->lens.focal35equivalent = false;
	ui->ccdWidth->setDisabled(project->lens.focal35equivalent);
	ui->ccdHeight->setDisabled(project->lens.focal35equivalent);
}

void FocalDialog::setAsEquivalent() {
	project->lens.focal35equivalent = true;
	ui->ccdWidth->setDisabled(project->lens.focal35equivalent);
	ui->ccdHeight->setDisabled(project->lens.focal35equivalent);
}
