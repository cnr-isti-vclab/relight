#include "lightgeometry.h"
#include "relightapp.h"
#include "directionsview.h"
#include "domepanel.h"
#include "../src/lp.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QFileDialog>

#include <QDebug>

#include <vector>

DomePanel::DomePanel(QWidget *parent): QFrame(parent) {

	setContentsMargins(10, 10, 10, 10);
	QGridLayout *content = new QGridLayout(this);
	content->setHorizontalSpacing(20);

	QPushButton *load = new QPushButton(QIcon::fromTheme("folder"), "Load dome file...");
	load->setProperty("class", "large");
	load->setMinimumWidth(200);
	load->setMaximumWidth(300);
	connect(load, SIGNAL(clicked()), this, SLOT(loadDomeFile()));
	content->addWidget(load, 1, 0, Qt::AlignTop);




	content->addWidget(new QLabel("Recent domes: "), 0, 1);
	dome_list = new QListWidget;
	content->addWidget(dome_list, 1, 1);


	connect(dome_list, SIGNAL(itemSelectionChanged()), this, SLOT(setSelectedDome()));
	init();
}

void DomePanel::init() {
	dome = qRelightApp->project().dome;
	updateDomeList();
}

void DomePanel::updateDomeList() {
	dome_labels.clear();
	dome_paths.clear();
	dome_list->clear();
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
	dome_list->clearSelection();
}


void DomePanel::loadLP(QString path) {
	std::vector<QString> filenames;
	dome = Dome();
	dome.lightConfiguration = Dome::DIRECTIONAL;

	try {
		parseLP(path, dome.directions, filenames);
	} catch(QString error) {
		QMessageBox::critical(this, "Loading .lp file failed", error);
		return;
	}
	QFileInfo info(path);
	dome.label = info.filePath();
	qRelightApp->addDome(path);
	updateDomeList();

	emit accept(dome);
}

void DomePanel::loadDome(QString path) {
	try {
		dome.load(path);
	} catch (QString error) {
		QMessageBox::critical(this, "Loading .dome file failed", error);
		return;
	}

	QFileInfo info(path);
	dome.label = info.filePath();
	qRelightApp->addDome(path);
	updateDomeList();
	emit accept(dome);
}



void DomePanel::setSelectedDome() {
	auto list = dome_list->selectedItems();
	if(!list.size())
		return;
	int pos = dome_list->row(list[0]);
	loadDomeFile(dome_paths[pos]);
}
