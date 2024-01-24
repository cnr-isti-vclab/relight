#ifndef RELIGHTAPP_H
#define RELIGHTAPP_H

#include <QApplication>
#include <QTemporaryDir>
#include <QMessageBox>
#include <QFile>
#include <QSettings>
#include <QVariant>
#include "../src/project.h"
#include "../relight/processqueue.h"

#define qRelightApp (static_cast<RelightApp *>(QCoreApplication::instance()))

class RecentProject {
public:
	QString filename;
	QString title;
	QString notes;
	QVariant toVariant();
	void fromVariane(QVariant &v);
};

class RelightApp: public QApplication {
public:
	Project m_project;

	RelightApp(int &argc, char **argv): QApplication(argc, argv) {
		QTemporaryDir tmp;
		if(!tmp.isValid()) {
			QMessageBox::critical(nullptr, "Temporary folder is needed", "Could not create a temporary file for the scripts.\nSelect a folder in File->Preferences");
		}

		QFile style(":/css/style.txt");
		style.open(QFile::ReadOnly);
		setStyleSheet(style.readAll());

		ProcessQueue &queue = ProcessQueue::instance();
		queue.start();

	}

	Project &project() { return m_project; }
	QString lastProjectDir() {
		return QSettings().value("LastProjectDir", QDir::homePath()).toString();
	}
	void setLastProjectDir(QString dir) {
		QSettings().setValue("LastProjectDir", dir);
	}
	std::vector<RecentProject> recents() {
		QList<QVariant> list = QSettings().value("Recents", QList<QVariant>()).toList();
		std::vector<RecentProject> m_recents;
		for(QVariant &v: list) {
			auto r = v.toMap();
			RecentProject p;
			p.filename = r["filename"].toString();
		}

		return m_recents;
	}
	void addRecent(QString filename, QString title, QString notes) {

	}
	void clearRecents() {

	}
};

#endif
