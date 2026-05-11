#ifndef IMAGEFRAME_H
#define IMAGEFRAME_H

#include <QFrame>
#include <QGraphicsScene>

class Canvas;
class ImageList;
class ImageGrid;
class ImageView;

class QStatusBar;
class QGraphicsPixmapItem;
class QGraphicsScene;
class QListWidgetItem;
class QToolBar;
class FlowLayout;


class ImageFrame: public QFrame {
	Q_OBJECT
public:
	enum Mode { SINGLE, LIST, THUMBNAILS };
	ImageList *image_list = nullptr;
	ImageGrid *image_grid = nullptr;
	ImageView *image_view = nullptr;
	//Canvas *canvas = nullptr;
	QStatusBar *status = nullptr;
	QToolBar *left_toolbar;
	QToolBar *center_toolbar;
	QToolBar *right_toolbar;

	ImageFrame(QWidget *parent = nullptr);
	void clear();
	void init();
	int currentImage();
	virtual void showImage(int id);
	void rotate(bool clockwise);

public slots:
	void rotateLeft(); //rotate counterclockwise selected images
	void rotateRight();
	virtual void previousImage();
	virtual void nextImage();
	void showImageItem(QListWidgetItem *item);

	void imageMode();
	void listMode();
	void gridMode();

	virtual void updateSkipped(int n);

signals:
	void skipChanged();
};

#endif // IMAGEFRAME_H
