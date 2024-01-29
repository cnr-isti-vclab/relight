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
	void setupActions();
	void createMenu();
	void initInterface(); //initialize interface using the current project
	void setTabIndex(int index);
	void setTabWidget(QWidget *widget);


	TabWidget *tabs;
	HomeFrame *home_frame;
	ImageFrame *image_frame;
};

#endif // MAINWINDOW_H
