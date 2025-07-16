#include "brdfplan.h"
#include "helpbutton.h"
#include "relightapp.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>

BrdfPlanRow::BrdfPlanRow(BrdfParameters &_parameters, QFrame *parent):
	PlanRow(parent), parameters(_parameters) {

}

BrdfMedianRow::BrdfMedianRow(BrdfParameters &_parameters, QFrame *parent):
	BrdfPlanRow(_parameters, parent) {

	label->label->setText("Median image");
	buttons->addWidget(new QLabel("Light percentage:"));
	buttons->addWidget(median_slider = new QSlider(Qt::Horizontal));
	buttons->addWidget(median_text = new QLabel);
	median_slider->setMinimum(0);
	median_slider->setMaximum(100);
	int percent = int(parameters.median_percentage);
	median_slider->setSliderPosition(percent);
	median_text->setText(QString::number(percent));


	connect(median_slider, &QSlider::valueChanged, [this](int v) {
		parameters.median_percentage = v;
		median_text->setText(QString::number(v));
	});
}


BrdfExportRow::BrdfExportRow(BrdfParameters &_parameters, QFrame *parent):
	BrdfPlanRow(_parameters, parent) {
	label->label->setText("Directory:");
	label->help->setId("brdf/export");

	path_edit = new QLineEdit;
	connect(path_edit, &QLineEdit::editingFinished,this, &BrdfExportRow::verifyPath);
	buttons->addWidget(path_edit);
	QPushButton *path_button = new QPushButton("...");
	buttons->addWidget(path_button);
	connect(path_button, &QPushButton::clicked, this, &BrdfExportRow::selectOutput);
}

void BrdfExportRow::setPath(QString path, bool emitting) {
	path_edit->setText(path);
	parameters.path = path;
}

void BrdfExportRow::verifyPath() {
	parameters.path = QString();
	QString path = path_edit->text();
	QDir path_dir(path);
	path_dir.cdUp();
	if(!path_dir.exists()) {
		QMessageBox::warning(this, "Invalid output path", "The specified path is not valid");
		return;
	}
	if(!path.endsWith(".jpg") && !path.endsWith(".png")) {
		path += ".jpg";
		path_edit->setText(path);
	}
	parameters.path = path;
}

void BrdfExportRow::selectOutput() {
	//get folder if not legacy.
	QString output_parent = qRelightApp->lastOutputDir();

	QString output = QFileDialog::getSaveFileName(this, "Select a file name", output_parent);
	if(output.isNull()) return;

	if(!output.endsWith(".jpg") && !output.endsWith(".png"))
			output += ".jpg";

	QDir output_parent_dir(output);
	output_parent_dir.cdUp();
	qRelightApp->setLastOutputDir(output_parent_dir.absolutePath());
	setPath(output);
}


void BrdfExportRow::suggestPath() {
	QDir input = qRelightApp->project().dir;
	input.cdUp();
	QString filename = input.filePath("brdf");
	setPath(filename);
}

