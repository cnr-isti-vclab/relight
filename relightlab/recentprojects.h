#ifndef RECENTPROJECTS_H
#define RECENTPROJECTS_H

#include <QString>
#include <vector>

class RecentProject {
public:
	QString filename;
	QString title;
	QString notes;

	QVariant toVariant();
	void fromVariant(QVariant &v);
};

static std::vector<RecentProject> getRecentProjects();
static void addRecentProject(QString filename, QString title, QString notes);
static void clearRecentProjects();


#endif // RECENTPROJECTS_H
