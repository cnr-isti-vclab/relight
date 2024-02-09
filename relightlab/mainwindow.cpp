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
#include "lightsframe.h"


#include <QMessageBox>

#include <iostream>
using namespace std;

MainWindow::MainWindow() {
	setWindowTitle("RelightLab");

	createMenu();

	home_frame = new HomeFrame;
	image_frame = new ImageFrame;
	lights_frame = new LightsFrame;
	tabs = new TabWidget;
	tabs->addTab(home_frame, "Home");
	tabs->addTab(image_frame, "Images");
	tabs->addTab(new QWidget, "Align");
	tabs->addTab(lights_frame, "Lights");
	tabs->addTab(new QWidget, "Crop");

	setCentralWidget(tabs);
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


void MainWindow::init() {
	image_frame->init();
	lights_frame->init();
}


