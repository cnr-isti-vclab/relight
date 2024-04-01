#include "rtiexportdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QRadioButton>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>

RtiExportDialog::RtiExportDialog(QWidget *parent): QDialog(parent) {
	setWindowTitle("Export RTI");
	setMinimumWidth(400);

	QVBoxLayout *content = new QVBoxLayout(this);

	QGroupBox *format = new QGroupBox("Format");
	content->addWidget(format);
	QVBoxLayout *format_layout = new QVBoxLayout(format);
	format_layout->addWidget(new QRadioButton("Plain images"));
	format_layout->addWidget(new QRadioButton("Deepzoom"));
	format_layout->addWidget(new QRadioButton("Tarzoom"));
	format_layout->addWidget(new QRadioButton("Interleaved Tarzoom"));
	
	QHBoxLayout *quality_box = new QHBoxLayout;
	content->addLayout(quality_box);
	quality_box->addWidget(new QLabel("Jpeg quality"));
	quality_box->addWidget(quality = new QSpinBox);
	quality->setRange(75, 100);


	content->addSpacing(20);
	content->addWidget(new QLabel("Directory:"));

	QHBoxLayout *dir = new QHBoxLayout;
	content->addLayout(dir);
	dir->addWidget(new QLineEdit());
	dir->addWidget(new QPushButton("..."));

	QHBoxLayout *buttons = new QHBoxLayout;
	content->addLayout(buttons);
	buttons->addStretch();
	buttons->addWidget(new QPushButton("Create"));
	buttons->addWidget(new QPushButton("Cancel"));
}

LegacyExportDialog::LegacyExportDialog(QWidget *parent): QDialog(parent) {
	setWindowTitle("Export legacy RTI");
	setMinimumWidth(400);

	QVBoxLayout *content = new QVBoxLayout(this);

	content->addWidget(new QRadioButton("Uncompressed"));
	content->addWidget(new QRadioButton("JPEG"));

	QHBoxLayout *quality_box = new QHBoxLayout;
	content->addLayout(quality_box);
	quality_box->addWidget(new QLabel("Jpeg quality"));
	quality_box->addWidget(quality = new QSpinBox);
	quality->setRange(75, 100);

	QHBoxLayout *filename = new QHBoxLayout;
	content->addLayout(filename);
	filename->addWidget(new QLineEdit());
	filename->addWidget(new QPushButton("..."));

	QHBoxLayout *buttons = new QHBoxLayout;
	content->addLayout(buttons);
	buttons->addStretch();
	buttons->addWidget(new QPushButton("Create"));
	buttons->addWidget(new QPushButton("Cancel"));
}
