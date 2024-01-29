#ifndef IMAGEFRAME_H
#define IMAGEFRAME_H

#include <QFrame>

class Canvas;
class QStatusBar;
class QGraphicsPixmapItem;
class QGraphicsScene;

class ImageFrame: public QFrame {
	Q_OBJECT
public:
	enum Mode { SINGLE, LIST, THUMBNAILS };
	Canvas *canvas = nullptr;
	QStatusBar *status = nullptr;
	ImageFrame();

	void showImage(int id); //new project loaded.

public slots:
	void fit();  //fit image on screen
	void one();  //scale to 1:1 zoom
private:
	QGraphicsPixmapItem *imagePixmap = nullptr;
	QGraphicsScene *scene = nullptr;
};

#endif // IMAGEFRAME_H
