#include <QSettings>
#include <QVariant>
#include <QMap>
#include <QList>
#include "recentprojects.h"

QStringList recentProjects() {
	return QSettings().value("recent-projects", QStringList()).toStringList();
}

void addRecentProject(const QString &filename) {
	QStringList recents = recentProjects();
	int index = recents.indexOf(filename);
	if (index != -1) { // String found in the list
		recents.removeAt(index); // Remove the string from its current position
	}
	recents.prepend(filename); // Add the string to the front
	QSettings().setValue("recent-projects", recents);
}

void clearRecentProjects() {
	QSettings().setValue("recent-projects", QStringList());
}


