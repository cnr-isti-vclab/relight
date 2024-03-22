#ifndef ALIGNFRAME_H
#define ALIGNFRAME_H

#include <QFrame>

class ImageView;

class AlignFrame: public QFrame {
public:
	AlignFrame(QWidget *parent = 0);

private:
	ImageView *image_view;
};

#endif // ALIGNFRAME_H
