#include <QMenuBar>
#include <QMenu>
#include <QSettings>
#include <QFileDialog>

#include "mainwindow.h"
#include "relightapp.h"
#include "tabwidget.h"
#include "homeframe.h"
#include "imageframe.h"

#include <iostream>
using namespace std;

MainWindow::MainWindow() {
	setWindowTitle("RelightLab");

	createMenu();

	home_frame = new HomeFrame;
	image_frame = new ImageFrame;
	tabs = new TabWidget;
	tabs->addTab(home_frame, "Home");
	tabs->addTab(image_frame, "Images");
	tabs->addTab(new QWidget, "Align");
	tabs->addTab(new QWidget, "Lights");
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
	menuFile->addSeparator();
	menuFile->addAction(qRelightApp->action("save_project"));
	menuFile->addAction(qRelightApp->action("save_project_as"));
	menuFile->addSeparator();
	menuFile->addAction(qRelightApp->action("exit"));

	setMenuBar(menubar);
}



void MainWindow::initInterface() {

}


