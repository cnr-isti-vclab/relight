#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include "sphere.h"
#include "project.h"
#include "dstretchdialog.h"
#include "zoomdialog.h"

#include <QProgressDialog>
#include <QGraphicsScene>
#include <QThreadPool>
#include <QFutureWatcher>
#include <QTimer>
#include <map>


class QListWidgetItem;
class QGraphicsEllipseItem;
class QGraphicsPixmapItem;
class QSettings;
class RtiExport;
class HelpDialog;
class QStandardItemModel;
class QStandardItem;
class QueueWindow;
class SettingsDialog;
class ConvertDialog;
class AlignDialog;
class AlignMarker;

namespace Ui {
class MainWindow;
}
/*
 * used just for debugging and working on the style. */
class AutoStyle: public QTimer {
	Q_OBJECT
public:
	AutoStyle() {
		this->setInterval(1000);
		this->start();
		setSingleShot(false);
		connect(this, SIGNAL(timeout()), this, SLOT(resetStyleSlot()));
	}
public slots:
	void resetStyleSlot() {
		//QFile style(":/darkorange/stylesheet.txt");
		QFile style("/home/ponchio/devel/relight/relight/darkorange/stylesheet.txt");
		style.open(QFile::ReadOnly);
		emit resetStyle(style.readAll());
	}
signals:
	void resetStyle(QString str);
};


class RTIScene: public QGraphicsScene {
	Q_OBJECT
public:
	RTIScene(QObject *parent = Q_NULLPTR): QGraphicsScene(parent) {}

signals:
	void borderPointMoved(QGraphicsEllipseItem *point);
	void highlightMoved(QGraphicsEllipseItem *highlight);
};

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow();

	Project project;
	QString project_filename;

	int currentImage = -1;
	//std::vector<bool> valid; //valid images.

	std::vector<int> progress_jobs;
	QProgressDialog *progress = nullptr;
	QueueWindow *queue = nullptr;
	SettingsDialog *settings_dialog = nullptr;
	ConvertDialog *convert = nullptr;
	QFutureWatcher<void> watcher;
	bool highlightDetecting = false;


	bool init();
	int detectHighlight(int n);

public slots:
	void enableActions();

	void openProject();
	void newProject();
	void saveProject();
	void saveProjectAs();
	void preferences();


	void clear(); //clear everything (project, images etc.)
	void openImage(const QModelIndex &current);
	void openImage(int id, bool fit = false);
	void imageChecked(QStandardItem *item);

	void next();
	void previous();
	void doubleClick(QPoint p);
	void pointClick(QPoint p);


/* Markers */

	void deleteSelected();

	void detectHighlights();
	void detectCurrentSphereHighlight();
	void cancelDetectHighlights();
	void finishedDetectHighlights();

	void updateBorderPoints(QGraphicsEllipseItem *point);
	void updateHighlight(QGraphicsEllipseItem *highlight);
	void showHighlights(size_t n);
	void setupSpheres();
	void newSphere();
	void removeSphere();

	void setupMeasures();
	void newMeasure();
	void removeMeasure();

	void setupAligns();
	void newAlign();
	void removeAlign();

	void setupWhites();
	void newWhite();
	void removeWhite();

	void showAlignDialog(AlignMarker *marker);


	
	void toggleMaxLuma();
	void computeMaxLuma();
	bool lumaCallback(std::string s, int n);
	void lumaCancel();
	void lumaFinish();
	


	void saveLPs();
	void loadLP();
	void loadLP(QString filename);
	void exportRTI(bool normals = false);

	void exportNormals();
	void convertRTI();
    void deepZoom();
    void tarZoom();
    void itarZoom();
    void dStretch();

	void viewRTI();
	void showQueue();

	void editLensParameters();
	void whiteBalance();
	void domeCalibration();

	void showHelp();

	void quit();
	
signals:
	void lumaProgressText(const QString &str);
	void lumaProgress(int n);
	

private:
	Ui::MainWindow *ui;
	QSettings *settings = nullptr;
	RtiExport *rtiexport = nullptr;
	HelpDialog *help = nullptr;
    ZoomDialog* zoom = nullptr;
    DStretchDialog* dstretch = nullptr;
	RTIScene *scene = nullptr;
	QGraphicsPixmapItem *imagePixmap = nullptr;
	
	bool ignore_scene_changes = false;
	int sphere_to_process = -1;
	QStringList notloaded; //used for looking for highlights and return status.
	QStringList flipped;
	QStringList resolution;
	QImage maxLuma;
	bool maxLuming = false;
	bool lumaCancelling = false;

	QStandardItemModel *imageModel = nullptr;
};


#endif // MAINWINDOW_H
