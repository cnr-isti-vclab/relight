#ifndef SPHERE_PICKING_H
#define SPHERE_PICKING_H

#include "imageframe.h"

class Canvas;

class SpherePicking: public ImageFrame {
public:
	SpherePicking(QWidget *parent = nullptr);

public slots:
	void cancel();
	void accept();
	void showImage(int id);

};

#endif
