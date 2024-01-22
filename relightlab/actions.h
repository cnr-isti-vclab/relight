#ifndef ACTIONS_H
#define ACTIONS_H

#include <QAction>

class Action {
public:
	static void initialize();
	static void cleanup();

	static QAction *new_project;
	static QAction *open_project;
	static QAction *close_project;
};

#endif // ACTIONS_H
