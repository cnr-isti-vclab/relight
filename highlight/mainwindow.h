#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>

class QListWidgetItem;
class QGraphicsScene;
class QGraphicsEllipseItem;
class QSettings;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	QDir dir;
	QStringList images;
	std::vector<bool> valid; //valid images.
	QSize imgsize;


	bool init(QString folder);

public slots:
	void open();
	void openImage(QListWidgetItem *);
	void exit();

private:
	Ui::MainWindow *ui;
	QSettings *settings;
	QGraphicsScene *scene = nullptr;
	QGraphicsEllipseItem *circle = nullptr;
};

#endif // MAINWINDOW_H
