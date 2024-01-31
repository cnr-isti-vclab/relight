#include "relightapp.h"
#include "../relight/processqueue.h"
#include "imageframe.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QAction>


RelightApp::RelightApp(int &argc, char **argv): QApplication(argc, argv) {
	QTemporaryDir tmp;
	if(!tmp.isValid()) {
		QMessageBox::critical(nullptr, "Temporary folder is needed", "Could not create a temporary file for the scripts.\nSelect a folder in File->Preferences");
	}

	QFile style(":/css/style.qss");
	style.open(QFile::ReadOnly);
	setStyleSheet(style.readAll());
	//Default font size can be read using QApplication::font().pointSize(), not pointPixel

	ProcessQueue &queue = ProcessQueue::instance();
	queue.start();


	addAction("new_project", "New project..", "", "Ctrl+N", SLOT(newProject()));
	addAction("open_project", "Open project...", "", "Ctrl+O", SLOT(openProject()));
	addAction("save_project", "Save project", "", "Ctrl+S", SLOT(saveProject()));
	addAction("save_project_as", "Save project as...", "", "Shift-Ctrl+S", SLOT(saveProjectAs()));

	addAction("preferences", "Preferences...", "", "Shift-Ctrl-P", SLOT(openPreferences()));
	addAction("exit", "Exit", "", "Alt-F4", SLOT(close()));

	//imagesframe
	addAction("zoom_fit", "Fit", "", "=");
	addAction("zoom_one", "Zoom 1x", "", "1");
	addAction("zoom_in", "Zoom in", "", "+");
	addAction("zoom_out", "Zoom out", "", "-");

	addAction("previous_image", "Previous image", "", "left");
	addAction("next_image", "Next image", "", "right");

	addAction("rotate_left", "Rotate left", "", "");
	addAction("rotate_right", "Rotate right", "", "");

	addAction("show_image", "Show image", "", "");
	addAction("show_list", "Show list", "", "");
	addAction("show_grid", "Show grid", "", "");
}
RelightApp::~RelightApp() {
}

void RelightApp::run() {
	mainwindow = new MainWindow;
	mainwindow->showMaximized();
}

void RelightApp::newProject() {
	if(!needsSavingProceed())
		return;

	QString dir = QFileDialog::getExistingDirectory(mainwindow, "Choose picture folder", qRelightApp->lastProjectDir());
	if(dir.isNull()) return;


	Project project;
	project.setDir(QDir(dir));
	bool ok = project.scanDir();
	if(!project.size()) {
		QMessageBox::critical(mainwindow, "Houston we have a problem!", "Could not find images in directory: " + project.dir.path());
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
			int answer = QMessageBox::question(mainwindow, "Some images are rotated.", "Do you wish to uniform image rotation?", QMessageBox::Yes, QMessageBox::No);
			if(answer != QMessageBox::No)
				project.rotateImages();
		} else
			QMessageBox::critical(mainwindow, "Resolution problem", "Not all of the images in the folder have the same resolution,\nyou might need to fix this problem manually.");
	}

	qRelightApp->project() = project;

	mainwindow->initInterface();

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
	mainwindow->setTabIndex(1);
}

void RelightApp::openProject() {
	if(!needsSavingProceed())
		return;
	QString filename = QFileDialog::getOpenFileName(mainwindow, "Select a project", qRelightApp->lastProjectDir(), "*.relight");
	if(filename.isNull())
		return;

	Project project;
	try {
		project.load(filename);
	} catch(QString e) {
		QMessageBox::critical(mainwindow, "Could not load the project: " + filename, "Error: " + e);
		return;
	}


	while(project.missing.size() != 0) {

		QString msg = "Could not find this images:\n";
		for(int i: project.missing)
			msg += "\t" + project.images[i].filename + "\n";

		QMessageBox box(mainwindow);
		box.setText(msg);
		box.setWindowTitle("Missing images");
		box.addButton("Ignore missing images", QMessageBox::AcceptRole);
		box.addButton("Select a different folder...", QMessageBox::ActionRole);
		box.addButton("Cancel", QMessageBox::RejectRole);
		int ret = box.exec();
		cout << "RET: " << ret << " " << (int)QMessageBox::ActionRole << endl;
		switch(ret) {
		case 1: {
			QString imagefolder = QFileDialog::getExistingDirectory(mainwindow, "Could not find the images, please select the image folder:", project.dir.absolutePath());
			if(imagefolder.isNull()) {
				QMessageBox::critical(mainwindow, "No folder selected", "No folder selected.");
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

	mainwindow->initInterface();
	mainwindow->setTabIndex(1);
	mainwindow->image_frame->init();
}

void RelightApp::saveProject() {

}

void RelightApp::saveProjectAs() {

}

void RelightApp::openPreferences() {

}

void RelightApp::close() {
	if(!needsSavingProceed())
			return;
	auto &q = ProcessQueue::instance();
	if(q.hasTasks()) {
		auto answer = QMessageBox::critical(mainwindow, "Process queue", "There are some processes in the queue, do you want to terminate them?");
		if(answer != QMessageBox::Yes)
			return;
	}
	q.stop();
	q.wait();
	exit(0);
}


bool RelightApp::needsSavingProceed() {
	if(!m_project.needs_saving)
		return true;
	auto answer = QMessageBox::question(mainwindow, "Current project is unsaved", "Do you want to proceed without saving?");
	return answer == QMessageBox::Yes;
}

QAction *RelightApp::addAction(const QString &id, const QString &label, const QString &icon, const QString &shortcut, const char *method) {
	QAction *a = new QAction(label);
	a->setShortcut(shortcut);
	actions[id] = a;
	if(method)
		connect(a, SIGNAL(triggered()), this, method);
	return a;
}
