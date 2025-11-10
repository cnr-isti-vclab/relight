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
	Project *m_project = nullptr;
	std::vector<QImage> m_thumbnails;

	QMap<QString, QAction *> actions;
	MainWindow *mainwindow = nullptr;
	Preferences *preferences = nullptr;
	QSystemTrayIcon *systemTray = nullptr;
	bool darkTheme = false;

	RelightApp(int &argc, char **argv);
	virtual ~RelightApp() { delete m_project; }
	void run();

public slots:
	void newProject();
	void openProject();
	void openProject(const QString &filename);
	void saveProject();
	void saveProjectAs();
	void close();
	void rtiView();
	void convertRTI();

	void openPreferences();
	void notify(const QString &title, const QString &msg, int ms = 4000);
signals:
	void updateThumbnail(int pos);

public:
	void setProject(Project *project);
	Project &project() { return *m_project; }

	QMutex thumbails_lock;
	std::vector<QImage> &thumbnails() { return m_thumbnails; }

	QAction *addAction(const QString &id, const QString &label, const QString &icon, const QString &shortcut, const char *method = nullptr);
	QAction *action(const QString &id) { return actions[id]; }


	QString lastOutputDir() {
		if(!last_output_dir.isEmpty())
			return last_output_dir;
		QDir out = m_project->dir;
		out.cdUp();
		return out.absolutePath();
	}
	void setLastOutputDir(QString out) {
		last_output_dir = out;
	}
	void clearLastOutputDir() {
		last_output_dir = QString();
	}


	QString lastProjectDir() {
		return QSettings().value("LastProjectDir", QDir::homePath()).toString();
	}
	void setLastProjectDir(QString dir) {
		QSettings().setValue("LastProjectDir", dir);
	}

	QString lastViewDir() {
		return QSettings().value("LastViewDir", QDir::homePath()).toString();
	}
	void setLastViewDir(QString dir) {
		QSettings().setValue("LastViewDir", dir);
	}

	//zero means auto
	int nThreads() {
		return QSettings().value("NThreads", 0).toInt();
	}

	void setThreads(int n) {
		QSettings().setValue("NThreads", n);
	}

	int samplingRam() {
		return QSettings().value("SamplingRam", 512).toInt();
	}
	void setSamplingRam(int mb) {
		QSettings().setValue("SamplingRam", mb);
	}

	int castingPort() {
		return QSettings().value("CastingPort", 8880).toInt();
	}
	void setCastingPort(int port) {
		QSettings().setValue("CastingPort", port);
	}

	QString defaultDome() {
		return QSettings().value("DefaultDome", QString()).toString();
	}
	void setDefaultDome(const QString &path) {
		QSettings().setValue("DefaultDome", path);
	}

	bool needsSavingProceed();

	QStringList domes();
	void addDome(QString filename);
	void removeDome(QString filename);
	void loadThumbnails();

private:
	//keep memory of current project filename for quick saving.
	QString project_filename;
	QString last_output_dir;
	QPalette dark_palette;
	ThumbailLoader *loader = nullptr;
};



#endif
