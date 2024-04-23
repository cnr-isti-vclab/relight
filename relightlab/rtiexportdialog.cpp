#include "rtiexportdialog.h"
#include "helpbutton.h"

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
	format_layout->addWidget(new HelpRadio("Plain images", "format/relight"));
	format_layout->addWidget(new HelpRadio("Deepzoom", "format/deepzoom"));
	format_layout->addWidget(new HelpRadio("Tarzoom", "format/tarzoom"));
	format_layout->addWidget(new HelpRadio("Interleaved Tarzoom", "format/itarzoom"));

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

	//seupt connections
	//connect(buttons->itemAt(1)->widget(), &QPushButton::clicked, this, &RtiExportDialog::accept);
	//connect(buttons->itemAt(2)->widget(), &QPushButton::clicked, this, &RtiExportDialog::reject);

	//init values from settings
	//int quality = QSettings().value("rti/defaults/quality", 95).toInt();
}

LegacyExportDialog::LegacyExportDialog(QWidget *parent): QDialog(parent) {
	setWindowTitle("Export legacy RTI");
	setMinimumWidth(400);

	QVBoxLayout *content = new QVBoxLayout(this);

	content->addWidget(new HelpRadio("Uncompressed", "format/compression"));
	content->addWidget(new HelpRadio("JPEG", "format/compressiob"));

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
