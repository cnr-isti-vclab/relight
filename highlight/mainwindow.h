#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include "ball.h"

#include <QGraphicsScene>

#include <map>


class QListWidgetItem;
class QGraphicsEllipseItem;
class QGraphicsPixmapItem;
class QSettings;


namespace Ui {
class MainWindow;
}

class RTIScene: public QGraphicsScene {
	Q_OBJECT
public:
	RTIScene(QObject *parent = Q_NULLPTR): QGraphicsScene(parent) {}

signals:
	void borderPointMoved();
};

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	QDir dir;
	QStringList images;
	std::vector<bool> valid; //valid images.
	QSize imgsize;
	int currentImage = -1;
	std::map<int, Ball> balls;


	bool init(QString folder);

public slots:

	void open();
	void openImage(QListWidgetItem *, bool fit = false);
	void next();
	void previous();
	void pointPicked(QPoint p);
	void deleteSelected();
	void updateBalls();

	void addSphere();
	void removeSphere();

	void process();
	void quit();

private:
	Ui::MainWindow *ui;
	QSettings *settings;
	RTIScene *scene = nullptr;
	QGraphicsPixmapItem *imagePixmap = nullptr;
	bool ignore_scene_changes = false;
};

#endif // MAINWINDOW_H
