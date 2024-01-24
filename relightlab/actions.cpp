#include "actions.h"

QAction *Action::new_project = nullptr;
QAction *Action::open_project = nullptr;
QAction *Action::close_project = nullptr;
QAction *Action::exit = nullptr;

void Action::initialize() {
	new_project = new QAction("New project...");
	new_project->setShortcut(QKeySequence("Ctrl+N"));

	open_project = new QAction("Open project...");
	open_project->setShortcut(QKeySequence("Ctrl+O"));

	close_project = new QAction("Close project...");
	close_project->setShortcut(QKeySequence("Ctrl+W"));
	close_project->setEnabled(false);

	exit = new QAction("Exit");
	exit->setShortcut(QKeySequence("Alt+F4"));
}

void Action::cleanup() {
	delete new_project;
	delete open_project;
	delete close_project;
	delete exit;
}
