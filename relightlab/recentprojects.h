#ifndef RECENTPROJECTS_H
#define RECENTPROJECTS_H

#include <QStringList>
#include <vector>


QStringList recentProjects();
void addRecentProject(const QString &filename);
void clearRecentProjects();


#endif // RECENTPROJECTS_H
