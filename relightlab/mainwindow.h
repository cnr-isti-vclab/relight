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
	void createMenu();
	void clear();

public slots:
	void newProject();
	void openProject();
	void closeProject();

private:
	TabWidget *tabs;
	QString project_filename; //last filename project opened or saved filename.
};

#endif // MAINWINDOW_H
