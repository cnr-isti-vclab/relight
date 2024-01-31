#include "preferences.h"
#include "tabwidget.h"

#include <QVBoxLayout>
#include <QPushButton>

Preferences::Preferences(QWidget *parent): QDialog(parent) {
	setWindowTitle("Preferences - RelightLab");

	tabs = new TabWidget;

	QWidget *appearance = new QWidget;
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
