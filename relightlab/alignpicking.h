#ifndef ALIGN_PICKING_H
#define ALIGN_PICKING_H

#include "imageview.h"
#include <QGraphicsRectItem>

class QGraphicsRectItem;
class Canvas;
class Align;

class AlignPicking;

class AlignRect: public QGraphicsItem {
public:
	int side;
	QRectF rect;

	AlignRect(AlignPicking *_picker, int _side, QGraphicsItem *parent = Q_NULLPTR):
		QGraphicsItem(parent), side(_side), picker(_picker) {
		setCursor(Qt::CrossCursor);
		setFlag(QGraphicsItem::ItemIsMovable);
		setFlag(QGraphicsItem::ItemIsSelectable);
		setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
		rect = QRectF(-side/2.0f, -side/2.0f, side, side);

	}
	virtual ~AlignRect() {}
	QRect getRect(); //return the portion of the image selected
	QRectF boundingRect() const override;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;
	void setRect(QRectF r);

protected:

	AlignPicking *picker;
	QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
};


class AlignPicking: public ImageViewer {
	Q_OBJECT
public:
	int marker_side = 40;
	//Align *align = nullptr;

	AlignRect *rect = nullptr;

	AlignPicking(QWidget *parent = nullptr);
	void setAlign(QRectF r);
	void updateAlign();
	void clear();
	QRectF getRect() { return rect->getRect(); }

public slots:
	void click(QPoint);
	//void updateAlignPoint();

protected:
	void keyPressEvent(QKeyEvent *event) override;
};

#endif
