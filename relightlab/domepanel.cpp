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

	QVBoxLayout *content = new QVBoxLayout(this);

	content->addWidget(new QLabel("<h3>Select an existing dome configuration</h3>"));
	content->addSpacing(30);


	QHBoxLayout *columns = new QHBoxLayout();
	content->addLayout(columns);

	QVBoxLayout *selection = new QVBoxLayout();
	columns->addLayout(selection, 1);

	QPushButton *load = new QPushButton(QIcon::fromTheme("folder"), "Load dome file...");
	load->setProperty("class", "large");
	load->setMinimumWidth(200);
	load->setMaximumWidth(300);
	connect(load, SIGNAL(clicked()), this, SLOT(loadDomeFile()));

	selection->addWidget(load);

	selection->addWidget(new QLabel("Recent domes: "));
	dome_list = new QListWidget;
	selection->addWidget(dome_list);


	QFrame *properties = new QFrame;
	columns->addWidget(properties, 2);

	QGridLayout *properties_layout = new QGridLayout(properties);
	properties_layout->addWidget(new QLabel("Filename: "), 0, 0, Qt::AlignRight);
	properties_layout->addWidget(filename = new QLabel, 0, 1);
	filename->setTextInteractionFlags(Qt::TextSelectableByMouse);

	properties_layout->addWidget(new QLabel("Label: "), 1, 0, Qt::AlignRight);
	properties_layout->addWidget(label = new QLineEdit, 1, 1);

	properties_layout->addWidget(new QLabel("Number of lights: "), 2, 0, Qt::AlignRight);
	properties_layout->addWidget(number = new QLabel, 2, 1);

	properties_layout->addWidget(new QLabel("Images: "), 3, 0, Qt::AlignRight);
	properties_layout->addWidget(images = new QListWidget, 3, 1);
	images->setMaximumWidth(600);
	images->setStyleSheet( "QListWidget{ background: palette(alternate-base); }");


	directions_view = new DirectionsView;
	columns->addWidget(directions_view, 1);
	directions_view->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	directions_view->setMaximumSize(200, 200);
	directions_view->setMinimumSize(200, 200);



	connect(dome_list, SIGNAL(itemSelectionChanged()), this, SLOT(setSelectedDome()));
	connect(label, &QLineEdit::textChanged, [&]() { dome.label = label->text(); emit accept(dome); });
	connect(images, SIGNAL(currentRowChanged(int)), directions_view, SLOT(highlight(int)));

	init();
}

void DomePanel::init() {
	dome_labels.clear();
	dome_paths.clear();
	dome_list->clear();

	//get list of existing domes
	QStringList paths = qRelightApp->domes();
	for(QString path: paths) {
		Dome dome;
		try {
			dome.load(path);
		} catch (QString error) {
			qDebug() << error;
		}
		if(dome.label.isEmpty()) {
			QFileInfo info(path);
			dome.label = info.fileName();
		}
		dome_labels.append(dome.label);
		dome_paths.append(path);
		dome_list->addItem(dome.label);
	}
}

void DomePanel::loadDomeFile() {
	QString path = QFileDialog::getOpenFileName(this, "Load a .lp or .dome file", QDir::currentPath(), "Light directions and domes (*.lp *.dome )");
	if(path.isNull())
		return;
	if(path.endsWith(".lp"))
		loadLP(path);
	if(path.endsWith(".dome"))
		loadDome(path);
	dome_list->clearSelection();
}

void DomePanel::update(QString path) {
	label->setText(dome.label);
	filename->setText(path);
	number->setText(QString::number(dome.directions.size()));
	directions_view->initFromDome(dome);

	emit accept(dome);
}

void DomePanel::loadLP(QString path) {
	std::vector<QString> filenames;

	dome.lightConfiguration = Dome::DIRECTIONAL;

	try {
		parseLP(path, dome.directions, filenames);
	} catch(QString error) {
		QMessageBox::critical(this, "Loading .lp file failed", error);
		return;
	}
	QFileInfo info(path);
	dome.label = info.fileName();

	images->clear();
	for(QString s: filenames)
		images->addItem(s);

	update(path);
}

void DomePanel::loadDome(QString path) {
	try {
		dome.load(path);
	} catch (QString error) {
		qDebug() << error;
	}

	images->clear();
	for(size_t i = 0; i < dome.directions.size(); i++) {
		images->addItem("Image " + QString::number(i));
	}

	update(path);
}


void DomePanel::setSelectedDome() {
	auto list = dome_list->selectedItems();
	if(!list.size())
		return;
	int pos = dome_list->row(list[0]);
	loadDome(dome_paths[pos]);
}
