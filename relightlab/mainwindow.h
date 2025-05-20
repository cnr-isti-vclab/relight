#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "normalsframe.h"
class QStackedWidget;
class TabWidget;
class HomeFrame;
class ImageFrame;
class AlignFrame;
class ScaleFrame;
class SphereFrame;
class LightsFrame;
class CropFrame;
class RtiFrame;
class BrdfFrame;
class NormalsFrame;
class QueueFrame;


class MainWindow: public QMainWindow {
	Q_OBJECT
public:
	MainWindow();
	void createMenu();
	void updateRecentProjectsMenu();
	void openRecentProject();

	void clear(); //clean up interface and stops project related tasks
	void init(); //initialize interface using the current project
	void setTabIndex(int index);
	void setTabWidget(QWidget *widget);

public slots:
	void showQueue();
	void showHelp();
	void showAbout();


protected:

	TabWidget *tabs = nullptr;
	HomeFrame *home_frame = nullptr;
	ImageFrame *image_frame = nullptr;
	AlignFrame *align_frame = nullptr;
	ScaleFrame *scale_frame = nullptr;
	SphereFrame *sphere_frame = nullptr;
	LightsFrame *lights_frame = nullptr;
	CropFrame *crop_frame = nullptr;
	RtiFrame *rti_frame = nullptr;
	BrdfFrame *brdf_frame = nullptr;
	NormalsFrame *normals_frame = nullptr;
	QueueFrame *queue_frame = nullptr;

	QMenu *recentMenu = nullptr;
};

#endif // MAINWINDOW_H
