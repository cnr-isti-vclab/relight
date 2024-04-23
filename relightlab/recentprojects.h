#ifndef RECENTPROJECTS_H
#define RECENTPROJECTS_H

#include <QStringList>

QStringList recentProjects();
void addRecentProject(const QString &filename);
void clearRecentProjects();

#endif // RECENTPROJECTS_H
