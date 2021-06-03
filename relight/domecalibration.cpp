#include "domecalibration.h"
#include "ui_domecalibration.h"

#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>

DomeCalibration::DomeCalibration(QWidget *parent, Dome &_dome) :
	QDialog(parent),
	dome(_dome),
	ui(new Ui::DomeCalibration) {
	ui->setupUi(this);
	ui->imageWidth->setValue(dome.imageWidth);
	ui->domeDiameter->setValue(dome.domeDiameter);
	ui->verticalOffset->setValue(dome.verticalOffset);
	switch(dome.lightConfiguration) {
	case Dome::DIRECTIONAL:
		ui->directional->setChecked(true);
		break;
	case Dome::SPHERICAL:
		ui->spherical->setChecked(true);
		break;
	case Dome::LIGHTS3D:
		ui->lights3d->setChecked(true);
		break;
	}
	update();
	connect(ui->imageWidth, SIGNAL(valueChanged(double)), this, SLOT(update()));
	connect(ui->domeDiameter, SIGNAL(valueChanged(double)), this, SLOT(update()));
	connect(ui->verticalOffset, SIGNAL(valueChanged(double)), this, SLOT(update()));

	connect(ui->directional, SIGNAL(clicked(bool)), this, SLOT(update()));
	connect(ui->spherical, SIGNAL(clicked(bool)), this, SLOT(update()));
	connect(ui->lights3d, SIGNAL(clicked(bool)), this, SLOT(update()));
}

DomeCalibration::~DomeCalibration() {
	delete ui;
}

void DomeCalibration::update() {
	bool directional = ui->directional->isChecked();
	bool spherical = ui->spherical->isChecked();

	ui->imageWidth->setEnabled(!directional);
	ui->domeDiameter->setEnabled(spherical);
	ui->verticalOffset->setEnabled(spherical);

	if(directional)
		dome.lightConfiguration = Dome::DIRECTIONAL;
	else if(spherical)
		dome.lightConfiguration = Dome::SPHERICAL;
	else
		dome.lightConfiguration = Dome::LIGHTS3D;

	dome.imageWidth = ui->imageWidth->value();
	dome.domeDiameter = ui->domeDiameter->value();
	dome.verticalOffset = ui->verticalOffset->value();
}


void DomeCalibration::loadConfig() {
	QString filename = QFileDialog::getOpenFileName(this, "Select light direction file", QString(), "Light directions (*.lp)");
	if(filename.isNull())
		return;

	QFile file(filename);
	if(!file.open(QFile::ReadOnly))
		throw QString("Failed opening: " + filename);

	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	QJsonObject obj = doc.object();

}

void DomeCalibration::saveConfig() {

}

