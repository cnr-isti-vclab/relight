#ifndef ALIGN_PICKING_H
#define ALIGN_PICKING_H

#include "imageview.h"

class QGraphicsRectItem;
class Canvas;
class Align;


class AlignPicking: public ImageViewer {
	Q_OBJECT
public:
	int marker_side = 40;
	Align *align = nullptr;

	QGraphicsRectItem *rect = nullptr;


	AlignPicking(QWidget *parent = nullptr);
	void setAlign(Align *sphere);
	void updateAlign();
	void clear();

public slots:
	void click(QPoint);

};

#endif
