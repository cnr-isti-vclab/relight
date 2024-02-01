#include "preferences.h"
#include "tabwidget.h"
#include "relightapp.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QCheckBox>

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
	QVBoxLayout *content = new QVBoxLayout;
	QCheckBox *dark = new QCheckBox("Dark theme (requires restart)");
	dark->setChecked(QSettings().value("dark", true).toBool());
	content->addWidget(dark);
	widget->setLayout(content);

	connect(dark, SIGNAL(toggled(bool)), qRelightApp, SLOT(setDarkTheme(bool)));
	return widget;
}
