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


class RelightApp: public QApplication {
public:
	Project m_project;
	QMap<QString, QAction *> actions;

	RelightApp(int &argc, char **argv);

	Project &project() { return m_project; }
	QAction *action(const QString &id) { return actions[id]; }

	QString lastProjectDir() {
		return QSettings().value("LastProjectDir", QDir::homePath()).toString();
	}
	void setLastProjectDir(QString dir) {
		QSettings().setValue("LastProjectDir", dir);
	}
protected
	:
	void addAction(const QString &id, const QString &label, const QString &shortcut);
};

#endif
