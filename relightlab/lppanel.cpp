#include "lppanel.h"
#include "relightapp.h"
#include "directionsview.h"
#include "../src/lp.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QListWidget>
#include <QDebug>


LpPanel::LpPanel() {

	QGridLayout *content = new QGridLayout(this);

	QFrame *lp_file = new QFrame;
	content->addWidget(lp_file);


	QPushButton *lp_load = new QPushButton(QIcon::fromTheme("folder"), "Load LP file...");
	lp_load->setProperty("class", "large");
	lp_load->setMinimumWidth(200);
	lp_load->setMaximumWidth(300);
	connect(lp_load, SIGNAL(clicked()), this, SLOT(loadLP()));

	content->addWidget(lp_load, 0, 0);

	directions_view = new DirectionsView;
	directions_view->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	directions_view->setMaximumSize(200, 200);
	directions_view->setMinimumSize(200, 200);

	content->addWidget(directions_view, 1, 0, 3, 1, Qt::AlignTop);

	QLabel *lp_filename_label = new QLabel("Filename: ");
	content->addWidget(lp_filename_label, 1, 1);
	lp_filename_label->setAlignment(Qt::AlignRight);

	content->addWidget(lp_filename = new QLabel(), 1, 2);
	lp_filename->setEnabled(false);

	QLabel *lp_number_label = new QLabel("Number of lights: ");
	content->addWidget(lp_number_label, 2, 1);
	lp_number_label->setAlignment(Qt::AlignRight);

	content->addWidget(lights_number = new QLabel(), 2, 2);
	lights_number->setEnabled(false);

	QLabel *lp_images_label = new QLabel("Images: ");
	content->addWidget(lp_images_label, 3, 1);
	lp_images_label->setAlignment(Qt::AlignRight);

	content->addWidget(images = new QListWidget, 3, 2);
	images->setMaximumWidth(600);
	images->setStyleSheet( "QListWidget{ background: palette(alternate-base); }");


	connect(images, SIGNAL(currentRowChanged(int)), directions_view, SLOT(highlight(int)));


	content->setColumnStretch(2, 10);
	content->setColumnStretch(3, 1);
	content->setRowStretch(2, 1);
	content->setHorizontalSpacing(30);
}


void LpPanel::loadLP() {
	QString lp = QFileDialog::getOpenFileName(this, "Load an LP file", QDir::currentPath(), "Light directions (*.lp)");
	if(lp.isNull())
		return;
	std::vector<QString> filenames;

	Dome dome;
	dome.lightConfiguration = Dome::DIRECTIONAL;

	try {
		parseLP(lp, dome.directions, filenames);
	} catch(QString error) {
		QMessageBox::critical(this, "Loading .lp file failed", error);
		return;
	}
	lp_filename->setText(lp);
	lights_number->setText(QString::number(dome.directions.size()));
	directions_view->initFromDome(dome);

	for(QString s: filenames)
		images->addItem(s);

	emit accept(dome);
}

