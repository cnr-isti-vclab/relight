#ifndef RECENTPROJECTS_H
#define RECENTPROJECTS_H

#include <QStringList>

QStringList recentProjects();
void addRecentProject(const QString &filename);
void removeRecentProject(const QString &filename);
void cleanRecentProjects(); //removes missing projects
void clearRecentProjects();

#endif // RECENTPROJECTS_H
