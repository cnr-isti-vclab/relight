#include "lightgeometry.h"
#include "relightapp.h"
#include "directionsview.h"
#include "domepanel.h"
#include "../src/lp.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QMessageBox>
#include <QFileDialog>

#include <QDebug>

#include <vector>

DomePanel::DomePanel(QWidget *parent): QFrame(parent) {

	//	setContentsMargins(10, 10, 10, 10);
	QHBoxLayout *content = new QHBoxLayout(this);
	//content->setHorizontalSpacing(20);


	QPushButton *save = new QPushButton(QIcon::fromTheme("save"), "Export dome...");
	save->setProperty("class", "large");
	save->setMinimumWidth(200);
	save->setMaximumWidth(300);
	connect(save, SIGNAL(clicked()), this, SLOT(exportDome()));
	content->addWidget(save, 0, Qt::AlignTop);

	QPushButton *load = new QPushButton(QIcon::fromTheme("folder"), "Load dome file...");
	load->setProperty("class", "large");
	load->setMinimumWidth(200);
	load->setMaximumWidth(300);
	connect(load, SIGNAL(clicked()), this, SLOT(loadDomeFile()));
	content->addWidget(load, 0, Qt::AlignTop);


	dome_list = new QComboBox;
	dome_list->setProperty("class", "large");
	content->addWidget(dome_list, 1, Qt::AlignTop);
	connect(dome_list, SIGNAL(currentIndexChanged(int)), this, SLOT(setDome(int)));
	init();
}

void DomePanel::init() {
	dome = qRelightApp->project().dome;
	updateDomeList();
}

void DomePanel::updateDomeList() {
	//dome_labels.clear();
	dome_paths.clear();
	dome_list->clear();
	dome_list->addItem("Select a recent dome...");
	//get list of existing domes
	QStringList paths = qRelightApp->domes();
	for(QString path: paths) {
		Dome dome; //yep, same name for a class member
		try {
			dome.load(path);
		} catch (QString error) {
			qDebug() << error;
		}

		QFileInfo info(path);
		dome.label = info.filePath();

		dome_labels.append(dome.label);
		dome_paths.append(path);
		dome_list->addItem(dome.label);
	}
}

void DomePanel::setDome(int index) {
	if(index <= 0)
		return;
	loadDomeFile(dome_paths[index-1]); //First index is "Seelect a recent dome..."
}
void DomePanel::loadDomeFile() {
	QString path = QFileDialog::getOpenFileName(this, "Load a .lp or .dome file", QDir::currentPath(), "Light directions and domes (*.lp *.dome )");
	if(path.isNull())
		return;
	loadDomeFile(path);
}

void DomePanel::loadDomeFile(QString path) {
	if(path.endsWith(".lp"))
		loadLP(path);
	if(path.endsWith(".dome"))
		loadDome(path);
	//	dome_list->clearSelection();
}

void DomePanel::exportDome() {
	QString filename = QFileDialog::getSaveFileName(this, "Select a dome file", qRelightApp->lastProjectDir(), "*.dome *.lp");
	if(filename.isNull())
		return;

	Dome &dome = qRelightApp->project().dome;

	if(filename.endsWith(".lp")) {
		try {
			qRelightApp->project().saveLP(filename, dome.directions);
		} catch(QString error) {
			QMessageBox::critical(this, "Failed to save the .lp file", error);
			return;
		}

	} else {

		if(!filename.endsWith(".dome"))
			filename += ".dome";

		try {
			dome.save(filename);
		} catch(QString error) {
			QMessageBox::critical(this, "Failed to save the .dome file", error);
			return;
		}
	}
	qRelightApp->addDome(filename);
}


void DomePanel::loadLP(QString path) {
	std::vector<QString> filenames;
	std::vector<Eigen::Vector3f> directions;

	try {
		parseLP(path, directions, filenames);
		qRelightApp->project().validateDome(directions.size());

	} catch(QString error) {
		QMessageBox::critical(this, "Loading .lp file failed", error);
		return;
	}
	Dome &dome = qRelightApp->project().dome;
	dome.lightConfiguration = Dome::DIRECTIONAL;
	dome.directions = directions;
	QFileInfo info(path);
	dome.label = info.filePath();
	qRelightApp->addDome(path);

	updateDomeList();

	emit updated();
}

void DomePanel::loadDome(QString path) {
	float imageWidth = dome.imageWidth;
	Dome new_dome;
	try {
		new_dome.load(path);
		qRelightApp->project().validateDome(new_dome.directions.size());

	} catch (QString error) {
		QMessageBox::critical(this, "Loading .dome file failed", error);
		return;
	}
	Dome &dome = qRelightApp->project().dome;
	dome = new_dome;

	QFileInfo info(path);
	dome.label = info.filePath();
	qRelightApp->addDome(path);
	//preserve image width if we actually have a measurement.
	if(imageWidth != 0 && qRelightApp->project().measures.size() != 0)
		dome.imageWidth = imageWidth;
	updateDomeList();
	emit updated();
}



/*void DomePanel::setSelectedDome() {
	auto list = dome_list->selectedItems();
	if(!list.size())
		return;
	int pos = dome_list->row(list[0]);
	loadDomeFile(dome_paths[pos]);
}*/
