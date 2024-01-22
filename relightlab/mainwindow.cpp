#include "mainwindow.h"

#include "tabwidget.h"
#include "homeframe.h"
#include "imageframe.h"
#include "actions.h"

#include <iostream>
using namespace std;

MainWindow::MainWindow() {
	Action::initialize();
	setupActions();

	setWindowTitle("RelightLab");

	tabs = new TabWidget;
	tabs->addTab(new HomeFrame, "Home");
	tabs->addTab(new ImageFrame, "Images");
	tabs->addTab(new QWidget, "Align");
	tabs->addTab(new QWidget, "Lights");
	tabs->addTab(new QWidget, "Crop");

	setCentralWidget(tabs);
}

void MainWindow::setupActions() {
	connect(Action::new_project, SIGNAL(triggered(bool)), this, SLOT(newProject()));
	connect(Action::open_project, SIGNAL(triggered(bool)), this, SLOT(openProject()));
}

void MainWindow::newProject() {
	cout << "New Project" << endl;
	tabs->setCurrentIndex(1);
}

void MainWindow::openProject() {
	cout << "open Project" << endl;
	tabs->setCurrentIndex(1);
}

void MainWindow::closeProject() {
	cout << "close Project" << endl;
}
