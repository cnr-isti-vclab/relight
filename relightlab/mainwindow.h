#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QStackedWidget;
class TabWidget;

class MainWindow: public QMainWindow {
	Q_OBJECT
public:
	MainWindow();
	void setupActions();

public slots:
	void newProject();
	void openProject();
	void closeProject();

protected:
	TabWidget *tabs;
};

#endif // MAINWINDOW_H
