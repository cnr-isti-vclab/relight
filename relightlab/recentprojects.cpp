#include <QSettings>
#include <QVariant>
#include <QMap>
#include <QList>
#include "recentprojects.h"


/*QList<QVariant> list = QSettings().value("Recents", QList<QVariant>()).toList();
std::vector<RecentProject> m_recents;
for(QVariant &v: list) {
	auto r = v.toMap();
	RecentProject p;
	p.filename = r["filename"].toString();
}*/

QVariant RecentProject::toVariant() {
	QMap<QString, QVariant> recent;
	recent["filename"] = QVariant(filename);
	recent["title"] = QVariant(title);
	recent["notes"] = QVariant(notes);
	return recent;
}

void RecentProject::fromVariant(const QVariant &v) {
	QMap<QString, QVariant> recent = v.toMap();
	filename = recent["filename"].toString();
	title = recent["title"].toString();
	notes = recent["notes"].toString();
}

static std::vector<RecentProject> getRecentProjects() {
	QList<QVariant> recents = QSettings().value("RecentProjects", QList<QVariant>()).toList();

	std::vector<RecentProject> recentProjects;
	for(const QVariant &v: recents) {
		recentProjects.push_back(v);
	}
}
static void addRecentProject(QString filename, QString title, QString notes) {

}
static void clearRecentProjects() {

}

