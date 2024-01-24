#include "relightapp.h"

#include <QAction>


RelightApp::RelightApp(int &argc, char **argv): QApplication(argc, argv) {
	QTemporaryDir tmp;
	if(!tmp.isValid()) {
		QMessageBox::critical(nullptr, "Temporary folder is needed", "Could not create a temporary file for the scripts.\nSelect a folder in File->Preferences");
	}

	QFile style(":/css/style.txt");
	style.open(QFile::ReadOnly);
	setStyleSheet(style.readAll());

	ProcessQueue &queue = ProcessQueue::instance();
	queue.start();


	addAction("new_project", "New project...", "Ctrl+N");
	addAction("open_project", "Open project...", "Ctrl+O");
	addAction("close_project", "Close project", "Ctrl-W");
	addAction("exit", "Exit", "Alt+F4");
}

void RelightApp::addAction(const QString &id, const QString &label, const QString &shortcut) {
	QAction *a = new QAction(label);
	a->setShortcut(shortcut);
	actions[id] = a;
}
