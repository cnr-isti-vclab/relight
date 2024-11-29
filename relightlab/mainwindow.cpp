#include <QMenuBar>
#include <QMenu>
#include <QSettings>
#include <QFileDialog>

#include "mainwindow.h"
#include "relightapp.h"
#include "recentprojects.h"
#include "tabwidget.h"
#include "homeframe.h"
#include "imageframe.h"
#include "alignframe.h"
#include "scaleframe.h"
#include "lightsframe.h"
#include "cropframe.h"
#include "rtiframe.h"
#include "normalsframe.h"
#include "queueframe.h"


#include <QMessageBox>
#include <QDebug>
#include <QScrollArea>
#include <QVBoxLayout>

#include <iostream>
using namespace std;

MainWindow::MainWindow() {
	setWindowTitle("RelightLab");

	createMenu();

	tabs = new TabWidget;

	tabs->addTab(home_frame = new HomeFrame, "Home");
	tabs->addTab(image_frame = new ImageFrame, "Images");
	tabs->addTab(align_frame = new AlignFrame, "Align");
	tabs->addTab(scale_frame = new ScaleFrame, "Scale");
	tabs->addTab(lights_frame = new LightsFrame, "Lights");
	tabs->addTab(crop_frame = new CropFrame, "Crop");
	tabs->addTab(rti_frame = new RtiFrame, "RTI");
	tabs->addTab(normals_frame = new NormalsFrame, "Normals");
	tabs->addTab(queue_frame = new QueueFrame, "Queue");

	connect(scale_frame, SIGNAL(pixelSizeChanged()), lights_frame, SLOT(setPixelSize()));
	connect(rti_frame, SIGNAL(processStarted()), this, SLOT(showQueue()));
	connect(normals_frame, SIGNAL(processStarted()), this, SLOT(showQueue()));

	setCentralWidget(tabs);
}

void MainWindow::showQueue() {
	tabs->setCurrentIndex(8);
}

void MainWindow::setTabIndex(int index) {
	tabs->setCurrentIndex(index);
}
void MainWindow::setTabWidget(QWidget *widget) {
	tabs->setCurrentWidget(widget);
}

void MainWindow::createMenu() {
	QMenuBar *menubar = new QMenuBar(this);//
//	menubar->setGeometry(QRect(0, 0, 1069, 22));

	QMenu *menuFile = new QMenu(menubar);
	menuFile->setTitle("File");
	menubar->addAction(menuFile->menuAction());


	menuFile->addAction(qRelightApp->action("new_project"));
	menuFile->addAction(qRelightApp->action("open_project"));	

	recentMenu = new QMenu("&Recent Projects", this);
	menuFile->addMenu(recentMenu);
	updateRecentProjectsMenu();

	menuFile->addSeparator();
	menuFile->addAction(qRelightApp->action("save_project"));
	menuFile->addAction(qRelightApp->action("save_project_as"));
	menuFile->addSeparator();
	menuFile->addAction(qRelightApp->action("preferences"));
	menuFile->addSeparator();
	menuFile->addAction(qRelightApp->action("exit"));

	setMenuBar(menubar);
}

void MainWindow::updateRecentProjectsMenu() {
	recentMenu->clear();

	QStringList recents = recentProjects();

	int count = 1;
	for (const QString& project : recents) {
		QAction *action = new QAction(QString::number(count++) + " | " + project, this);
		action->setProperty("filename", project);
		connect(action, &QAction::triggered, this, &MainWindow::openRecentProject);
		recentMenu->addAction(action);
	}
}

void MainWindow::openRecentProject() {
	QAction *action = qobject_cast<QAction *>(sender());
	if (action) {
		QString projectFilename = action->property("filename").toString();
		if(!qRelightApp->needsSavingProceed())
			return;
		qRelightApp->openProject(projectFilename);
	}
}

void MainWindow::clear() {
	image_frame->clear();
	scale_frame->clear();
	lights_frame->clear();
	crop_frame->clear();
}

void MainWindow::init() {
	image_frame->init();
	scale_frame->init();
	lights_frame->init();
	crop_frame->init();
	rti_frame->init();
}


