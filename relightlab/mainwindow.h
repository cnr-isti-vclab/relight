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
	void initInterface(); //initialize interface using the current project

public slots:
	void newProject();
	void openProject();
	void closeProject();

private:
	TabWidget *tabs;
	QString project_filename; //last filename project opened or saved filename.

	bool needsSavingProceed(); //return false if userd oesn't want to proceed
};

#endif // MAINWINDOW_H
