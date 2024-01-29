#ifndef RELIGHTAPP_H
#define RELIGHTAPP_H

#include <QApplication>
#include <QTemporaryDir>
#include <QMessageBox>
#include <QFile>
#include <QSettings>
#include <QVariant>

#include "../src/project.h"
#include "mainwindow.h"

#define qRelightApp (static_cast<RelightApp *>(QCoreApplication::instance()))


class RelightApp: public QApplication {
	Q_OBJECT
public:
	Project m_project;

	QMap<QString, QAction *> actions;
	MainWindow *mainwindow = nullptr;

	RelightApp(int &argc, char **argv);
	~RelightApp();
	void run();

public slots:
	void newProject();
	void openProject();
	void saveProject();
	void saveProjectAs();
	void close();

	void openPreferences();

public:
	Project &project() { return m_project; }
	QAction *action(const QString &id) { return actions[id]; }

	QString lastProjectDir() {
		return QSettings().value("LastProjectDir", QDir::homePath()).toString();
	}
	void setLastProjectDir(QString dir) {
		QSettings().setValue("LastProjectDir", dir);
	}

private:
	QAction *addAction(const QString &id, const QString &label, const QString &icon, const QString &shortcut, const char *method = nullptr);
	bool needsSavingProceed();

	//keep memory of current project filename for quick saving.
	QString project_filename;
};

#endif
