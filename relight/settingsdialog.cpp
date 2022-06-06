#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
SettingsDialog::SettingsDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::SettingsDialog) {
	ui->setupUi(this);

	connect(ui->openPython, SIGNAL(clicked()), this, SLOT(openPython()));
	connect(ui->openScripts, SIGNAL(clicked()), this, SLOT(openScripts()));
	connect(ui->python, SIGNAL(editingFinished()), this, SLOT(setPython()));
	connect(ui->scripts, SIGNAL(editingFinished()), this, SLOT(setScripts()));
	connect(ui->port, SIGNAL(editingFinished()), this, SLOT(setPort()));
	connect(ui->ram, SIGNAL(editingFinished()), this, SLOT(setRam()));
	connect(ui->nworkers, SIGNAL(editingFinished()), this, SLOT(setWorkers()));

	QSettings settings;
	ui->python->setText(settings.value("python_path").toString());
	ui->scripts->setText(settings.value("scripts_path").toString());
	ui->port->setValue(settings.value("port", 8080).toInt());
	ui->ram->setValue(settings.value("ram", 512).toInt());
	ui->nworkers->setValue(settings.value("nworkers", 8).toInt());
}

SettingsDialog::~SettingsDialog() {
	delete ui;
}

void SettingsDialog::setPython() {
	QString python = ui->python->text();
	if(!checkPython(python))
		return;

	QSettings().setValue("python_path", python);
	ui->python->setText(python);
}
void SettingsDialog::setScripts() {
	QString scripts = ui->scripts->text();
	if(!scripts.isEmpty() && !checkScripts(scripts)) {
		ui->scripts->setText(QSettings().value("scripts_path").toString());
		return;
	}

	QSettings().setValue("scripts_path", scripts);
	ui->scripts->setText(scripts);
}

void SettingsDialog::setPort() {
	int port = ui->port->value();
	QSettings settings;
	settings.setValue("port", port);
}

void SettingsDialog::setRam() {
	int ram = ui->ram->value();
	QSettings settings;
	settings.setValue("ram", ram);
}


void SettingsDialog::setWorkers() {
	int nworkers = ui->nworkers->value();
	QSettings settings;
	settings.setValue("nworkers", nworkers);
}


void SettingsDialog::openPython() {
	QString python = QFileDialog::getOpenFileName(this, "Select python 3 executable");
	if(python.isNull())
		return;
	if(!checkPython(python))
		return;
	QSettings settings;
	settings.setValue("python_path", python);
	ui->python->setText(python);
}


void SettingsDialog::openScripts() {
	QString scripts = QFileDialog::getExistingDirectory(this, "Select scripts directory:");
	if(scripts.isNull())
		return;

	if(!checkScripts(scripts))
		return;

	QSettings().setValue("scripts_path", scripts);
	ui->scripts->setText(scripts);
}

bool SettingsDialog::checkPython(QString python) {
	QFileInfo info(python);
	if(!info.exists()) {
		QMessageBox::critical(this, "This file doesn't exist", "The path you wrote doesn't exists. Point to Python3 executable...");
		return false;
	}
	return true;
}

bool SettingsDialog::checkScripts(QString scripts) {
	QDir dir(scripts);
	QFileInfo info(dir.filePath("deepzoom.py"));
	if(!info.exists()) {
		QMessageBox::critical(this, "This folder might not be the correct one.",
							  "Could not locate the expected scripts in this folder. It should contain deepzoom.py, tarzoom.py, etc.");
		return false;
	}
	return true;
}
