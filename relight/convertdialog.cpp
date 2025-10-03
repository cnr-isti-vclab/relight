#include "convertdialog.h"
#include "ui_convertdialog.h"
#include <QFile>
#include <QMessageBox>
#include <QFileDialog>
#include "../src/rti/rtitask.h"
#include "processqueue.h"


ConvertDialog::ConvertDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ConvertDialog) {
	ui->setupUi(this);

	connect(ui->convert, SIGNAL(clicked()), this, SLOT(convert()));
	connect(ui->load, SIGNAL(clicked()), this, SLOT(selectInput()));


}

ConvertDialog::~ConvertDialog() {
	delete ui;
}

void ConvertDialog::selectInput() {
	QString filename = QFileDialog::getOpenFileName(this, "Select a .rti or .ptm file", QString(), tr("Relightable image (*.rti *.ptm)"));
	ui->filename->setText(filename);
}

void ConvertDialog::convert() {
	QString filename = ui->filename->text();
	if(filename.isEmpty())
		return;

	QFile file(filename);
	if(!file.exists()) {
		QMessageBox::critical(this, "Could not find the .rti file", "The file cannot be found!");
		return;
	}


	QString output = QFileDialog::getSaveFileName(this, "Select an output directory for relight", QString());
	if(output.isEmpty())
		return;


	RtiTask *task = new RtiTask(*project);
	task->output = output;
	task->label = "RTI"; //should use
	task->addParameter("input", Parameter::STRING, filename);
	QStringList steps;
	steps << "fromRTI";
	if(ui->deepzoom->isChecked())
		steps << "deepzoom";
	else if(ui->tarzoom->isChecked())
		steps << "deepzoom" << "tarzoom";
	else if(ui->itarzoom->isChecked())
		steps << "deepzoom" << "tarzoom" << "itarzoom";

	if(ui->openlime->isChecked())
		steps << "openlime";

	task->addParameter("steps", Parameter::STRINGLIST, steps);

	int quality = ui->quality->value();
	task->addParameter("quality", Parameter::INT, quality);

	ProcessQueue &queue = ProcessQueue::instance();
	queue.addTask(task);

	close();
}
