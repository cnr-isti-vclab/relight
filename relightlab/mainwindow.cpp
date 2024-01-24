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

	menuFile->addAction(qRelightApp->action("new_project"));
	menuFile->addAction(qRelightApp->action("open_project"));
	menuFile->addAction(qRelightApp->action("close_project"));

	menuFile->addSeparator();
	menuFile->addSeparator();
	menuFile->addAction(qRelightApp->action("exit"));

	setMenuBar(menubar);
}

void MainWindow::setupActions() {
	connect(qRelightApp->action("new_project"), SIGNAL(triggered(bool)), this, SLOT(newProject()));
	connect(qRelightApp->action("open_project"), SIGNAL(triggered(bool)), this, SLOT(openProject()));
	connect(qRelightApp->action("close_project"), SIGNAL(triggered(bool)), this, SLOT(closeProject()));

	connect(qRelightApp->action("exit"), SIGNAL(triggered(bool)), this, SLOT(close()));

}

void MainWindow::newProject() {
	if(!needsSavingProceed())
		return;

	QString dir = QFileDialog::getExistingDirectory(this, "Choose picture folder", qRelightApp->lastProjectDir());
	if(dir.isNull()) return;


	Project project;
	project.setDir(QDir(dir));
	bool ok = project.scanDir();
	if(!project.size()) {
		QMessageBox::critical(this, "Houston we have a problem!", "Could not find images in directory: " + project.dir.path());
		return;
	}

	if(!ok) {
		//check if we can rotate a few images.
		bool canrotate = false;
		for(Image &image: project.images) {
			if(image.size == project.imgsize)
				continue;

			if(image.isRotated(project.imgsize))
				canrotate = true;
		}
		if(canrotate) {
			int answer = QMessageBox::question(this, "Some images are rotated.", "Do you wish to uniform image rotation?", QMessageBox::Yes, QMessageBox::No);
			if(answer != QMessageBox::No)
				project.rotateImages();
		} else
			QMessageBox::critical(this, "Resolution problem", "Not all of the images in the folder have the same resolution,\nyou might need to fix this problem manually.");
	}

	qRelightApp->project() = project;

	initInterface();

/* TODO: move this into the lights tab
	QStringList img_ext;
	img_ext << "*.lp";
	QStringList lps = QDir(dir).entryList(img_ext);
	if(lps.size() > 0) {
		int answer = QMessageBox::question(this, "Found an .lp file: " + lps[0], "Do you wish to load " + lps[0] + "?", QMessageBox::Yes, QMessageBox::No);
		if(answer != QMessageBox::No)
			loadLP(lps[0]);
	}
*/

	qRelightApp->action("close_project")->setEnabled(true);
	tabs->setCurrentIndex(1);
}

void MainWindow::openProject() {
	if(!needsSavingProceed())
		return;
	QString filename = QFileDialog::getOpenFileName(this, "Select a project", qRelightApp->lastProjectDir(), "*.relight");
	if(filename.isNull())
		return;

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

	qRelightApp->project() = project;
	project_filename = filename; //project.dir.relativeFilePath(filename);
	qRelightApp->setLastProjectDir(project.dir.path());

	initInterface();
	qRelightApp->action("close_project")->setEnabled(true);

	tabs->setCurrentIndex(1);
}

void MainWindow::closeProject() {

	cout << "close Project" << endl;
	qRelightApp->action("close_project")->setEnabled(false);
}

void MainWindow::initInterface() {

}

bool MainWindow::needsSavingProceed() {
	if(!qRelightApp->project().needs_saving);
	return true;
	auto answer = QMessageBox::question(this, "Current project is unsaved", "Do you want to proceed without saving?");
	return answer == QMessageBox::Yes;
}
