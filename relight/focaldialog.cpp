#include "focaldialog.h"
#include "ui_focaldialog.h"
#include "project.h"

FocalDialog::FocalDialog(Project *_project, QWidget *parent) :
	QDialog(parent),
	project(_project),
	ui(new Ui::FocalDialog) {

	ui->setupUi(this);

	ui->width->setValue(project->imgsize.width());
	ui->height->setValue(project->imgsize.width());

	Lens &lens = project->lens;

	ui->focalx->setValue(lens.focalx);
	ui->focaly->setValue(lens.focaly);
	ui->ccdWidth->setValue(lens.ccdWidth);
	ui->ccdHeight->setValue(lens.ccdHeight);
	ui->principalOffsetX->setValue(lens.principalOffsetX);
	ui->principalOffsetY->setValue(lens.principalOffsetY);
	ui->k1->setValue(lens.k1);
	ui->k2->setValue(lens.k2);
	ui->p1->setValue(lens.p1);
	ui->p2->setValue(lens.p2);

	ui->equivalent->setChecked(lens.focal35equivalent);
	ui->real->setChecked(!lens.focal35equivalent);

	connect(ui->equivalent, SIGNAL(clicked(bool)), this, SLOT(setAsEquivalent()));
	connect(ui->real,       SIGNAL(clicked(bool)), this, SLOT(setAsReal()));
}

FocalDialog::~FocalDialog() {
	delete ui;
}

void FocalDialog::setAsReal() {
	project->lens.focal35equivalent = false;

}

void FocalDialog::setAsEquivalent() {
	project->lens.focal35equivalent = true;
}
