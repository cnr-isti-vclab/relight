#include <QMenuBar>
#include <QMenu>
#include <QSettings>
#include <QFileDialog>

#include "mainwindow.h"
#include "relightapp.h"
#include "tabwidget.h"
#include "homeframe.h"
#include "imageframe.h"
#include "actions.h"

#include <iostream>
using namespace std;

MainWindow::MainWindow() {
	Action::initialize();
	setupActions();
	createMenu();

	setWindowTitle("RelightLab");

	tabs = new TabWidget;
	tabs->addTab(new HomeFrame, "Home");
	tabs->addTab(new ImageFrame, "Images");
	tabs->addTab(new QWidget, "Align");
	tabs->addTab(new QWidget, "Lights");
	tabs->addTab(new QWidget, "Crop");

	setCentralWidget(tabs);
}

void MainWindow::createMenu() {
	QMenuBar *menubar = new QMenuBar(this);//
//	menubar->setGeometry(QRect(0, 0, 1069, 22));

	QMenu *menuFile = new QMenu(menubar);
	menuFile->setTitle("File");
	menubar->addAction(menuFile->menuAction());

	menuFile->addAction(Action::new_project);
	menuFile->addAction(Action::open_project);
	menuFile->addAction(Action::close_project);

	menuFile->addSeparator();
	menuFile->addSeparator();
	menuFile->addAction(Action::exit);

	setMenuBar(menubar);
}

void MainWindow::setupActions() {
	connect(Action::new_project, SIGNAL(triggered(bool)), this, SLOT(newProject()));
	connect(Action::open_project, SIGNAL(triggered(bool)), this, SLOT(openProject()));
	connect(Action::close_project, SIGNAL(triggered(bool)), this, SLOT(closeProject()));

	connect(Action::exit, SIGNAL(triggered(bool)), this, SLOT(close()));

}

void MainWindow::clear() {

}

void MainWindow::newProject() {
	cout << "New Project" << endl;
	Action::close_project->setEnabled(true);
	tabs->setCurrentIndex(1);
}

void MainWindow::openProject() {
	QString lastDir = QSettings().value("LastDir", QDir::homePath()).toString();

	QString filename = QFileDialog::getOpenFileName(this, "Select a project", lastDir, "*.relight");
	if(filename.isNull())
		return;

	clear();

	Project project;
	try {
		project.load(filename);
	} catch(QString e) {
		QMessageBox::critical(this, "Could not load the project: " + filename, "Error: " + e);
		return;
	}


	while(project.missing.size() != 0) {

		QString msg = "Could not find this images:\n";
		for(int i: project.missing)
			msg += "\t" + project.images[i].filename + "\n";

		QMessageBox box(this);
		box.setText(msg);
		box.setWindowTitle("Missing images");
		box.addButton("Ignore missing images", QMessageBox::AcceptRole);
		box.addButton("Select a different folder...", QMessageBox::ActionRole);
		box.addButton("Cancel", QMessageBox::RejectRole);
		int ret = box.exec();
		cout << "RET: " << ret << " " << (int)QMessageBox::ActionRole << endl;
		switch(ret) {
		case 1: {
			QString imagefolder = QFileDialog::getExistingDirectory(this, "Could not find the images, please select the image folder:", project.dir.absolutePath());
			if(imagefolder.isNull()) {
				QMessageBox::critical(this, "No folder selected", "No folder selected.");
				return;
			}
			project.dir.setPath(imagefolder);
			QDir::setCurrent(imagefolder);
			project.checkMissingImages();
			project.checkImages();
			}
			break;
		case 2: //cancel
			return;
		case 3: //ignore
			project.missing.clear();
			break;
		}
	}

	clear();
	qRelightApp->project() = project;
	project_filename = filename; //project.dir.relativeFilePath(filename);

	QSettings().setValue("LastDir", project.dir.path());

	Action::close_project->setEnabled(true);
	tabs->setCurrentIndex(1);
}

void MainWindow::closeProject() {
	cout << "close Project" << endl;
	Action::close_project->setEnabled(false);
}
