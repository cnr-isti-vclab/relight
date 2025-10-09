#include "relightapp.h"
#include "processqueue.h"
#include "imageframe.h"
#include "recentprojects.h"
#include "mainwindow.h"
#include "preferences.h"
#include "convertdialog.h"
#include "../src/network/httpserver.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QTemporaryDir>
#include <QStyleFactory>
#include <QStyle>
#include <QAction>
#include <QMessageBox>
#include <QProxyStyle>
#include <QSystemTrayIcon>
#include <QGuiApplication>
#include <QStyleHints>
#include <QPalette>

#include <iostream>
using namespace std;



QIcon ProxyStyle::standardIcon(StandardPixmap standardIcon,
								 const QStyleOption *option,
								 const QWidget *widget) const {
	switch(standardIcon) {
	case QStyle::SP_DialogOkButton:     return QIcon::fromTheme("check");
	case QStyle::SP_DialogCancelButton: return QIcon::fromTheme("cancel");
	case QStyle::SP_DialogHelpButton:   return QIcon::fromTheme("help-circle");
	case QStyle::SP_DialogSaveButton:   return QIcon::fromTheme("save");
	default: return QProxyStyle::standardIcon(standardIcon, option, widget);
	}
};


RelightApp::RelightApp(int &argc, char **argv): QApplication(argc, argv) {
	QTemporaryDir tmp;
	if(!tmp.isValid()) {
		QMessageBox::critical(nullptr, "Temporary folder is needed", "Could not create a temporary file for the scripts.\nSelect a folder in File->Preferences");
	}



	QFile style(":/css/style.qss");
	style.open(QFile::ReadOnly);
	setStyleSheet(style.readAll());
	//Default font size can be read using QApplication::font().pointSize(), not pointPixel

	dark_palette.setColor(QPalette::Window,QColor(53,53,53));
	dark_palette.setColor(QPalette::WindowText,Qt::white);
	dark_palette.setColor(QPalette::Disabled,QPalette::WindowText,QColor(127,127,127));
	dark_palette.setColor(QPalette::Disabled,QPalette::Text,QColor(127,127,127));
	dark_palette.setColor(QPalette::Base,QColor(42,42,42));
	dark_palette.setColor(QPalette::AlternateBase,QColor(66,66,66));
	dark_palette.setColor(QPalette::Button,QColor(53,53,53));
	dark_palette.setColor(QPalette::ToolTipBase,QColor(53,53,53));
	dark_palette.setColor(QPalette::ToolTipText,Qt::white);
	dark_palette.setColor(QPalette::Text,QColor(220, 220, 220));
	dark_palette.setColor(QPalette::Dark,QColor(35,35,35));
	dark_palette.setColor(QPalette::Shadow,QColor(20,20,20));

	dark_palette.setColor(QPalette::ButtonText,Qt::white);
	dark_palette.setColor(QPalette::Disabled,QPalette::ButtonText,QColor(127,127,127));
	dark_palette.setColor(QPalette::BrightText,Qt::red);
	dark_palette.setColor(QPalette::Link,QColor(42,130,218));
	dark_palette.setColor(QPalette::Highlight,QColor(42,130,218));
	dark_palette.setColor(QPalette::Disabled,QPalette::Highlight,QColor(80,80,80));
	dark_palette.setColor(QPalette::HighlightedText,Qt::white);
	dark_palette.setColor(QPalette::Disabled,QPalette::HighlightedText,QColor(127,127,127));

	this->setStyle(new ProxyStyle(QStyleFactory::create("Fusion")));

	setAttribute(Qt::AA_DontShowIconsInMenus);
	QIcon::setThemeSearchPaths(QStringList() << ":/icons");


	ProcessQueue &queue = ProcessQueue::instance();
	connect(&queue, SIGNAL(finished(QString, QString)), this, SLOT(notify(QString, QString)));
	queue.start();



	addAction("new_project", "New project..", "file", "Ctrl+N", SLOT(newProject()));
	addAction("open_project", "Open project...", "folder", "Ctrl+O", SLOT(openProject()));
	addAction("save_project", "Save project", "save", "Ctrl+S", SLOT(saveProject()));
	addAction("save_project_as", "Save project as...", "save", "Shift-Ctrl+S", SLOT(saveProjectAs()));
	addAction("convert_rti", "Convert RTI to Relight...", "repeat", "Ctrl+R", SLOT(convertRTI()));
	addAction("view_rti", "View RTI in browser", "cast", "", SLOT(rtiView()));

	addAction("preferences", "Preferences...", "", "Shift-Ctrl-P", SLOT(openPreferences()));
	addAction("exit", "Exit", "", "Alt-F4", SLOT(close()));

	//imagesframe
	addAction("zoom_fit", "Fit", "maximize", "=");
	addAction("zoom_one", "Zoom 1x", "", "1");
	addAction("zoom_in", "Zoom in", "zoom-in", "+");
	addAction("zoom_out", "Zoom out", "zoom-out", "-");

	addAction("previous_image", "Previous image", "chevron-left", "left");
	addAction("next_image", "Next image", "chevron-right", "right");

	addAction("rotate_left", "Rotate left", "rotate-ccw", "");
	addAction("rotate_right", "Rotate right", "rotate-cw", "");

	addAction("show_image", "Show image", "image", "");
	addAction("show_list", "Show list", "list", "");
	addAction("show_grid", "Show grid", "grid", "");


	addAction("help", "Help", "help-circle", "");
	addAction("about", "About", "info", "");


	if(QSystemTrayIcon::isSystemTrayAvailable()) {
		QIcon icon(":/relight.png");
		systemTray = new QSystemTrayIcon(icon, this);
		systemTray->show();
		systemTray->setVisible(false);
	}

	m_project = new Project;

}
void RelightApp::notify(const QString &title, const QString &msg, int ms) {
	if(!systemTray)
		return;
	QIcon icon(":/relight.png");
	systemTray->setVisible(true);
	systemTray->showMessage(title, msg, icon, ms);
	systemTray->setVisible(false);

}

inline bool isDarkMode() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
  const auto scheme = QGuiApplication::styleHints()->colorScheme();
  return scheme == Qt::ColorScheme::Dark;
#else
  const QPalette defaultPalette = qRelightApp->palette();
  const auto text = defaultPalette.color(QPalette::WindowText);
  const auto window = defaultPalette.color(QPalette::Window);
  return text.lightness() > window.lightness();
#endif // QT_VERSION
}

void RelightApp::run() {
	QString theme = QSettings().value("theme", "user").toString();

	if(theme == "user") {
		darkTheme = isDarkMode();
	} else if(theme == "dark") {
		darkTheme = true;
	}
	if(darkTheme) {
		QIcon::setThemeName("dark");
		setPalette(dark_palette);
	} else {
		QIcon::setThemeName("light");
	}

	mainwindow = new MainWindow;
	mainwindow->showMaximized();
}

void RelightApp::setProject(Project *_project) {
	//cleanup interface and stop (and remove) project related tasks.
	mainwindow->clear();

	delete m_project;

	m_project = _project;
	loadThumbnails();

	mainwindow->init();
	mainwindow->setTabIndex(1);
	qRelightApp->setLastProjectDir(m_project->dir.path());
	qRelightApp->clearLastOutputDir();
}

void RelightApp::newProject() {
	if(!needsSavingProceed())
		return;

	QString dir = QFileDialog::getExistingDirectory(mainwindow, "Choose picture folder", qRelightApp->lastProjectDir());
	if(dir.isNull()) return;


	Project *project = new Project;
	QDir folder(dir);
	if(!folder.exists()) {
		QString newDir = QFileDialog::getExistingDirectory(mainwindow, "Could not find the image folder: select the images folder.", dir);
		if(newDir.isNull()) {
			delete project;
			return;
		}
		folder.setPath(newDir);
		if(!folder.exists()) {
			QMessageBox::critical(mainwindow, "Error", "Could not find the image folder.");
			delete project;
			return;
		}
	}
	project->setDir(folder);
	bool ok = project->scanDir();
	if(!project->size()) {
		QMessageBox::critical(mainwindow, "Houston we have a problem!", "Could not find images in directory: " + project->dir.path());
		return;
	}

	if(!ok) {
		//check if we can rotate a few images.
		bool canrotate = false;
		for(Image &image: project->images) {
			if(image.size == project->imgsize)
				continue;

			if(image.isRotated(project->imgsize))
				canrotate = true;
		}
		if(canrotate) {
			int answer = QMessageBox::question(mainwindow, "Some images are rotated.", "Do you wish to uniform image rotation?", QMessageBox::Yes, QMessageBox::No);
			if(answer != QMessageBox::No)
				project->rotateImages();
		} else
			QMessageBox::critical(mainwindow, "Resolution problem", "Not all of the images in the folder have the same resolution,\nyou might need to fix this problem manually.");
	}

	project_filename = QString();


	//Check for .lp files in the folder
	QStringList img_ext;
	img_ext << "*.lp";
	QStringList lps = QDir(dir).entryList(img_ext);
	if(lps.size() > 0) {
		int answer = QMessageBox::question(mainwindow, "Found an .lp file: " + lps[0], "Do you wish to load " + lps[0] + "?", QMessageBox::Yes, QMessageBox::No);
		if(answer != QMessageBox::No) {
			try {
				bool loaded = project->loadLP(lps[0]);
				if(loaded) {
					QFileInfo info(lps[0]);
					addDome(info.filePath());
				}
			} catch(QString error) {
				QMessageBox::critical(mainwindow, "Could not load the .lp file", error);
			}
		}
	}

	qRelightApp->setProject(project);
}

void RelightApp::openProject() {
	if(!needsSavingProceed())
		return;
	QString filename = QFileDialog::getOpenFileName(mainwindow, "Select a project", qRelightApp->lastProjectDir(), "*.relight");
	if(filename.isNull())
		return;
	openProject(filename);
}

void RelightApp::openProject(const QString &filename) {

	QString current = QDir::currentPath();

	Project *project = new Project;
	try {
		project->load(filename);
	} catch(QString e) {
		QMessageBox::critical(mainwindow, "Could not load project", e);
		QDir::setCurrent(current);
		return;
	}

	while(project->missing.size() != 0) {
		QString msg = "Could not find these images:\n";
		for(int i: project->missing)
			msg += "\t" + project->images[i].filename + "\n";

		if(msg.size() > 300)
		msg = msg.left(297) + "...";

		QMessageBox box(mainwindow);
		box.setText(msg);
		box.setWindowTitle("Missing images");
		box.addButton("Select a different folder...", QMessageBox::ActionRole);
		box.addButton("Cancel", QMessageBox::RejectRole);
		int ret = box.exec();

		if(ret == 2) {
			QDir::setCurrent(current);
			return;
		}

		QString imagefolder = QFileDialog::getExistingDirectory(mainwindow, "Could not find the images, please select the image folder:", project->dir.absolutePath());
		if(imagefolder.isNull()) {
			QMessageBox::critical(mainwindow, "No folder selected", "No folder selected.");
			QDir::setCurrent(current);
			return;
		}
		project->dir.setPath(imagefolder);
		QDir::setCurrent(imagefolder);
		project->checkMissingImages();
		project->checkImages();
	}
	qRelightApp->setProject(project);

	project_filename = filename;
	addRecentProject(filename);
	mainwindow->updateRecentProjectsMenu();
}

void RelightApp::saveProject() {

	if(project_filename.isNull()) {
		QString filename = QFileDialog::getSaveFileName(mainwindow, "Save file: ", qRelightApp->lastProjectDir(), "*.relight");
		if(filename.isNull())
			return;
		if(!filename.endsWith((".relight")))
			filename += ".relight";
		project_filename = filename;
	}

	m_project->save(project_filename);

	QFileInfo info(project_filename);
	mainwindow->setWindowTitle("Relight - " + info.fileName());
	addRecentProject(project_filename);
	mainwindow->updateRecentProjectsMenu();
}

void RelightApp::saveProjectAs() {
	QString filename = QFileDialog::getSaveFileName(mainwindow, "Save file: ", qRelightApp->lastProjectDir(), "*.relight");
	if(filename.isNull())
		return;
	if(!filename.endsWith((".relight")))
		filename += ".relight";
	project_filename = filename;

	m_project->save(project_filename);
	QFileInfo info(project_filename);
	mainwindow->setWindowTitle("Relight - " + info.fileName());
	addRecentProject(project_filename);
	mainwindow->updateRecentProjectsMenu();
}

void RelightApp::loadThumbnails() {
	//if loading thumbails kill thumbnails
	if(loader) {
		loader->stop();
		loader->wait();
		delete loader;
		loader = nullptr;
	}
	m_thumbnails.resize(m_project->images.size());
	QStringList paths;
	for(size_t i = 0; i < m_project->images.size(); i++) {
		Image &image = m_project->images[i];
		if(i == 0) {
			QImage img;
			img.load(image.filename, "JPG");
			if(img.isNull()) {
				img = QImage(256, 256, QImage::Format_ARGB32);
				img.fill(Qt::black);
			}
			m_thumbnails[i] = img.scaledToHeight(256);
			emit updateThumbnail(0);
		} else {
			QImage img(m_thumbnails[0].size().scaled(256, 256, Qt::KeepAspectRatio), QImage::Format_ARGB32);
			img.fill(Qt::black);
			m_thumbnails[i] = img;
			paths.push_back(image.filename);
		}
	}

	//start a thumbnail loading thread and ask her to signal when something has been loaded.
	loader = new ThumbailLoader(paths);
	connect(loader, SIGNAL(update(int)), this, SIGNAL(updateThumbnail(int)));
	QObject::connect(this, &QCoreApplication::aboutToQuit, [&]() {
		if(loader) {
			loader->stop();
			loader->wait();
			delete loader;
			loader = nullptr;
		}
	});
	loader->start();

}

void RelightApp::rtiView() {
	QString last = lastViewDir();
	QString dirname = QFileDialog::getExistingDirectory(nullptr, "Select a folder containing an RTI in relight format: json and jpeg", last);
	if(dirname.isNull())
		return;
	QDir dir(dirname);
	QFileInfo info(dir.filePath("info.json"));
	if(!info.exists()) {
		QMessageBox::warning(nullptr, "Could not open relight folder", "It seems this folder do not contains an rti in relight format");
		return;
	}
	setLastViewDir(dirname);
	try {
		HttpServer &server = HttpServer::instance();
		server.stop();
		server.port = qRelightApp->castingPort();
		server.start(dirname);
		server.show();
	} catch(QString error) {
		QMessageBox::critical(nullptr, "Could not cast!", error);
	}
}

void RelightApp::convertRTI() {
	ConvertDialog dialog;
	if(dialog.exec() == QDialog::Accepted) {
		//add a task to the queue to convert the rti.
	}
}


void RelightApp::openPreferences() {
	if(!preferences)
		preferences = new Preferences(mainwindow);
	preferences->show();
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
	if(!m_project->needs_saving)
		return true;
	auto answer = QMessageBox::question(mainwindow, "Current project is unsaved", "Do you want to proceed without saving?");
	return answer == QMessageBox::Yes;
}

QStringList RelightApp::domes() {
	return QSettings().value("domes", QVariant(QStringList())).toStringList();
}

void RelightApp::addDome(QString filename) {
	QStringList d = domes();
	d.removeAll(filename);
	d.prepend(filename);
	QSettings().setValue("domes", d);
}

void RelightApp::removeDome(QString filename) {
	QStringList d = domes();
	d.removeAll(filename);
	QSettings().setValue("domes", d);
}


QAction *RelightApp::addAction(const QString &id, const QString &label, const QString &icon, const QString &shortcut, const char *method) {
	QAction *a = new QAction(label);
	a->setShortcut(shortcut);
	if(icon != "") {
		//a->setIcon(QIcon(icon));
		a->setIcon(QIcon::fromTheme(icon));
	}
	a->setObjectName(id);
	actions[id] = a;
	if(method)
		connect(a, SIGNAL(triggered()), this, method);
	return a;
}

ThumbailLoader::ThumbailLoader(QStringList &images) {
	QDir current = QDir::current();
	for(QString filename: images)
		paths.push_back(current.absoluteFilePath(filename));
}

void ThumbailLoader::run() {
	int count = 1;
	for(QString path: paths) {
		if(stop_request)
			break;
		QImage img;
		img.load(path, "JPG");
		if(img.isNull()) //TODO shoudl actually warn!
			break;
		{
			QMutexLocker lock(&qRelightApp->thumbails_lock);
			qRelightApp->thumbnails()[count] = img.scaledToHeight(256);;
		}
		emit update(count);
		count++;
	}
}
