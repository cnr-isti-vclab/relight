#ifndef IMAGEFRAME_H
#define IMAGEFRAME_H

#include <QFrame>

class Canvas;
class QStatusBar;

class ImageFrame: public QFrame {
public:
	Canvas *canvas = nullptr;
	QStatusBar *status = nullptr;
	ImageFrame();

public slots:

};

#endif // IMAGEFRAME_H
