#include "preferences.h"
#include "tabwidget.h"
#include "relightapp.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QFileDialog>
#include <QHBoxLayout>

Preferences::Preferences(QWidget *parent): QDialog(parent) {
	setWindowTitle("Preferences - RelightLab");
	setModal(true);
	setMinimumWidth(800);

	tabs = new TabWidget;

	tabs->addTab(buildAppearance(), "Appearance");
	tabs->addTab(buildPerformances(), "Performances");
	tabs->addTab(buildCasting(), "Casting");
	tabs->addTab(buildDome(), "Domes");

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(tabs);

	QPushButton *closeButton = new QPushButton("Close", this);
	layout->addWidget(closeButton);

	connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

QWidget *Preferences::buildCasting() {
	QWidget *widget = new QWidget;
	QGridLayout *layout = new QGridLayout(widget);
	layout->addWidget(new QLabel("Http port:"), 0, 0);
	QSpinBox *port = new QSpinBox;
	port->setMinimum(1);
	port->setMaximum(65535);
	port->setValue(qRelightApp->castingPort());

	layout->addWidget(port, 0, 1);

	connect(port, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [](int v) {
		qRelightApp->setCastingPort(v); });

	return widget;
}

QWidget *Preferences::buildPerformances() {
	QWidget *widget = new QWidget;

	QGridLayout *layout = new QGridLayout(widget);

	int n_threads = qRelightApp->nThreads();

	QCheckBox *all_cores = new QCheckBox("Use all CPU cores");
	all_cores->setChecked(n_threads == 0);
	layout->addWidget(all_cores, 0, 0);

	layout->addWidget(new QLabel("Number of threads:"), 1, 0);
	QSpinBox *threads = new QSpinBox;
	threads->setMinimum(1);
	threads->setMaximum(64);
	threads->setValue(n_threads == 0 ? 8 : n_threads);
	threads->setEnabled(n_threads != 0);
	layout->addWidget(threads, 1, 0);

	connect(all_cores, &QCheckBox::toggled, [threads](bool on) {
		qRelightApp->setThreads(on? 0 : threads->value());
		threads->setEnabled(!on);
	});

	connect(threads, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [](int v) {
		qRelightApp->setThreads(v); });

	return widget;
}

QWidget *Preferences::buildAppearance() {
	QWidget *widget = new QWidget;
	QVBoxLayout *content = new QVBoxLayout(widget);
	{
		QGroupBox *box = new QGroupBox("Set theme (requires restart)");

		{
			QRadioButton *light = new QRadioButton("Light");
			QRadioButton *dark = new QRadioButton("Dark");
			QRadioButton *user = new QRadioButton("System");

			if(QSettings().contains("dark")) {
				bool dark = QSettings().value("dark").toBool();
				QSettings().setValue("theme", dark ? "dark" : "user");
				QSettings().remove("dark");
			}

			QString theme = QSettings().value("theme", "user").toString();

			light->setChecked(theme == "light");
			dark->setChecked(theme == "dark");
			user->setChecked(theme == "user");

			connect(dark, &QRadioButton::toggled, [](bool on) { if(on) QSettings().setValue("theme", "dark"); });
			connect(light, &QRadioButton::toggled, [](bool on) { if(on) QSettings().setValue("theme", "light"); });
			connect(user, &QRadioButton::toggled, [](bool on) { if(on) QSettings().setValue("theme", "user"); });

			QVBoxLayout *buttons = new QVBoxLayout(box);
			buttons->addWidget(light);
			buttons->addWidget(dark);
			buttons->addWidget(user);
		}
		content->addWidget(box);
	}

	return widget;
}

QWidget *Preferences::buildDome() {
	QWidget *widget = new QWidget;
	QVBoxLayout *layout = new QVBoxLayout(widget);
	
	QLabel *description = new QLabel(
		"Specify a default dome file (.dome or .lp) to use when creating new projects.\n"
		"The default dome will be used if the number of lights matches the images in the project.\n"
		"If an .lp file exists in the image folder, you will be asked which one to use."
	);
	description->setWordWrap(true);
	layout->addWidget(description);
	
	QHBoxLayout *fileLayout = new QHBoxLayout;
	
	QLineEdit *domePathEdit = new QLineEdit;
	domePathEdit->setText(qRelightApp->defaultDome());
	domePathEdit->setReadOnly(true);
	domePathEdit->setPlaceholderText("No default dome set");
	
	QPushButton *browseButton = new QPushButton("Browse...");
	QPushButton *clearButton = new QPushButton("Clear");
	
	fileLayout->addWidget(new QLabel("Default dome file:"));
	fileLayout->addWidget(domePathEdit, 1);
	fileLayout->addWidget(browseButton);
	fileLayout->addWidget(clearButton);
	
	layout->addLayout(fileLayout);
	layout->addStretch();
	
	connect(browseButton, &QPushButton::clicked, [domePathEdit]() {
		QString filename = QFileDialog::getOpenFileName(
			nullptr,
			"Select Default Dome File",
			qRelightApp->lastProjectDir(),
			"Dome files (*.dome *.lp);;All files (*.*)"
		);
		
		if (!filename.isEmpty()) {
			domePathEdit->setText(filename);
			qRelightApp->setDefaultDome(filename);
			
			// Update last directory
			QFileInfo info(filename);
			qRelightApp->setLastProjectDir(info.absolutePath());
		}
	});
	
	connect(clearButton, &QPushButton::clicked, [domePathEdit]() {
		domePathEdit->clear();
		qRelightApp->setDefaultDome(QString());
	});
	
	return widget;
}
