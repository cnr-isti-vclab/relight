#include "preferences.h"
#include "tabwidget.h"
#include "relightapp.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QRadioButton>
#include <QGroupBox>

Preferences::Preferences(QWidget *parent): QDialog(parent) {
	setWindowTitle("Preferences - RelightLab");
	setModal(true);

	tabs = new TabWidget;

	QWidget *appearance = buildAppearance();
	tabs->addTab(appearance, "Appearance");

	QWidget *performances = new QWidget;
	tabs->addTab(performances, "Performances");

	QWidget *casting = new QWidget;
	tabs->addTab(casting, "Casting");

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(tabs);

	QPushButton *closeButton = new QPushButton("Close", this);
	layout->addWidget(closeButton);

	connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
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
