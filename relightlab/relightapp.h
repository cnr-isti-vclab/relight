#ifndef RELIGHTAPP_H
#define RELIGHTAPP_H

#include <QApplication>
#include <QTemporaryDir>
#include <QMessageBox>
#include <QFile>
#include <QSettings>
#include <QVariant>
#include <QProxyStyle>
#include "../src/project.h"


#define qRelightApp (static_cast<RelightApp *>(QCoreApplication::instance()))

class Preferences;
class MainWindow;


/* customize standard icons */
class ProxyStyle : public QProxyStyle {
	Q_OBJECT

public:
	ProxyStyle(QStyle *style = 0) : QProxyStyle(style) { }

public slots:
	QIcon standardIcon(StandardPixmap standardIcon,
									 const QStyleOption *option = 0,
									 const QWidget *widget = 0) const;
};


class RelightApp: public QApplication {
	Q_OBJECT
public:
	Project m_project;

	QMap<QString, QAction *> actions;
	MainWindow *mainwindow = nullptr;
	Preferences *preferences = nullptr;

	RelightApp(int &argc, char **argv);
	virtual ~RelightApp() {}
	void run();

public slots:
	void newProject();
	void openProject();
	void openProject(const QString &filename);
	void saveProject();
	void saveProjectAs();
	void close();

	void openPreferences();
	void setDarkTheme(bool on);

public:
	Project &project() { return m_project; }
	QAction *action(const QString &id) { return actions[id]; }
	QString lastProjectDir() {
		return QSettings().value("LastProjectDir", QDir::homePath()).toString();
	}
	void setLastProjectDir(QString dir) {
		QSettings().setValue("LastProjectDir", dir);
	}
	bool needsSavingProceed();

	QStringList domes();
	void addDome(QString filename);
	void removeDome(QString filename);

private:
	QAction *addAction(const QString &id, const QString &label, const QString &icon, const QString &shortcut, const char *method = nullptr);


	//keep memory of current project filename for quick saving.
	QString project_filename;
	QPalette dark_palette;
};

#endif
