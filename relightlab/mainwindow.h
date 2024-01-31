#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QStackedWidget;
class TabWidget;
class HomeFrame;
class ImageFrame;

class MainWindow: public QMainWindow {
	Q_OBJECT
public:
	MainWindow();
	void createMenu();
	void updateRecentProjectsMenu();
	void openRecentProject();

	void initInterface(); //initialize interface using the current project
	void setTabIndex(int index);
	void setTabWidget(QWidget *widget);


	TabWidget *tabs;
	HomeFrame *home_frame;
	ImageFrame *image_frame;
private:
	QMenu *recentMenu = nullptr;
};

#endif // MAINWINDOW_H
