#ifndef IMAGEFRAME_H
#define IMAGEFRAME_H

#include <QFrame>
#include <QGraphicsScene>

class Canvas;
class ImageList;
class ImageGrid;

class QStatusBar;
class QGraphicsPixmapItem;
class QGraphicsScene;
class QListWidgetItem;
class QToolBar;


class ImageFrame: public QFrame {
	Q_OBJECT
public:
	enum Mode { SINGLE, LIST, THUMBNAILS };
	ImageList *image_list = nullptr;
	ImageGrid *image_grid = nullptr;
	Canvas *canvas = nullptr;
	QStatusBar *status = nullptr;
	QToolBar *left_toolbar;
	QToolBar *center_toolbar;
	QToolBar *right_toolbar;

	ImageFrame(QWidget *parent = nullptr);
	void init();
	int currentImage();
	void showImage(int id); //new project loaded.

public slots:
	void fit();  //fit image on screen
	void one();  //scale to 1:1 zoom
	void previousImage();
	void nextImage();
	void showImageItem(QListWidgetItem *item);

	void imageMode();
	void listMode();
	void gridMode();

protected:
	QGraphicsScene scene;

private:
	QGraphicsPixmapItem *imagePixmap = nullptr;
	bool first_resize = true;
};

#endif // IMAGEFRAME_H
