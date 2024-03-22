#ifndef ALIGNFRAME_H
#define ALIGNFRAME_H

#include <QFrame>

class ImageViewer;
class QGraphicsRectItem;
class AlignFrame: public QFrame {
Q_OBJECT
public:
	AlignFrame(QWidget *parent = 0);

public slots:
	void click(QPoint p);

private:
	ImageViewer *image_viewer;
	std::vector<QGraphicsRectItem *> samples;
};

#endif // ALIGNFRAME_H
