#ifndef RECENTPROJECTS_H
#define RECENTPROJECTS_H

#include <QString>
#include <vector>

class RecentProject {
public:
	QString filename;
	QString title;
	QString notes;

	RecentProject() {}
	RecentProject(const QVariant &v) { fromVariant(v); }
	QVariant toVariant();
	void fromVariant(const QVariant &v);
};

static std::vector<RecentProject> getRecentProjects();
static void addRecentProject(QString filename, QString title, QString notes);
static void clearRecentProjects();


#endif // RECENTPROJECTS_H
