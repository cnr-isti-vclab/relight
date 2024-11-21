#ifndef RELIGHTAPP_H
#define RELIGHTAPP_H

#include "../src/project.h"

#include <QApplication>
#include <QList>
#include <QSettings>
#include <QVariant>
#include <QProxyStyle>
#include <QAction>
#include <QThread>
#include <QMutex>

#include <set>


#define qRelightApp (static_cast<RelightApp *>(QCoreApplication::instance()))

class Preferences;
class MainWindow;
class Task;
class QSystemTrayIcon;

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

class ThumbailLoader: public QThread {
	Q_OBJECT
public:
	ThumbailLoader(QStringList &images);
	void stop() { stop_request = true; }
signals:
	void update(int); //something has been loaded.

protected:
	virtual void run();

	QStringList paths;
	bool stop_request = false;
};

class RelightApp: public QApplication {
	Q_OBJECT
public:
	Project m_project;
	std::vector<QImage> m_thumbnails;

	QMap<QString, QAction *> actions;
	MainWindow *mainwindow = nullptr;
	Preferences *preferences = nullptr;
	QSystemTrayIcon *systemTray = nullptr;

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
	void notify(const QString &title, const QString &msg, int ms = 4000);
signals:
	void updateThumbnail(int pos);

public:
	void setProject(const Project &project);
	Project &project() { return m_project; }

	QMutex thumbails_lock;
	std::vector<QImage> &thumbnails() { return m_thumbnails; }

	QAction *addAction(const QString &id, const QString &label, const QString &icon, const QString &shortcut, const char *method = nullptr);
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
	void loadThumbnails();

private:


	//keep memory of current project filename for quick saving.
	QString project_filename;
	QPalette dark_palette;
	ThumbailLoader *loader = nullptr;
};



#endif
