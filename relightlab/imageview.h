#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

#include "canvas.h"
#include <QGraphicsScene>

class Canvas;
class QGraphicsPixmapItem;

class QToolBar;


class ImageView: public Canvas {
	Q_OBJECT
public:
	QGraphicsScene scene;
	int current_image = -1;

	ImageView(QWidget *parent = nullptr);
	void clear();

public slots:
	void showImage(int id);
	void fit();  //fit image on screen
	void one();  //scale to 1:1 zoom
	void next(); //show next image
	void prev(); //show previous image

private:
	QGraphicsPixmapItem *imagePixmap = nullptr;

};

class ImageViewer: public QFrame {
	Q_OBJECT
public:
	ImageView *view;

	ImageViewer(QWidget *parent = nullptr);
	QGraphicsScene &scene() { return view->scene; }

	void showImage(int id);
	int currentImage() { return view->current_image; }

public slots:
	void fit() { view->fit(); }
	void one() { view->one(); }
	void next() { view->next(); }
	void prev() { view->prev(); }
protected:

	QToolBar *toolbar;
};

#endif // IMAGEVIEW_H
