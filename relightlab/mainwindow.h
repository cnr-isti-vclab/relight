#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QStackedWidget;
class TabWidget;
class HomeFrame;
class ImageFrame;
class LightsFrame;
class CropFrame;
class RtiFrame;

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


	TabWidget *tabs = nullptr;
	HomeFrame *home_frame = nullptr;
	ImageFrame *image_frame = nullptr;
	LightsFrame *lights_frame = nullptr;
	CropFrame *crop_frame = nullptr;
	RtiFrame *rti_frame = nullptr;

private:
	QMenu *recentMenu = nullptr;
};

#endif // MAINWINDOW_H
