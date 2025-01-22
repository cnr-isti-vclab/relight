#ifndef ALIGN_PICKING_H
#define ALIGN_PICKING_H

#include "imageview.h"
#include <QGraphicsRectItem>

class QGraphicsRectItem;
class Canvas;
class Align;

class AlignPicking;

class AlignRect: public QGraphicsRectItem {
public:
	AlignRect(AlignPicking *_picker, qreal x, qreal y, qreal w, qreal h, QGraphicsItem *parent = Q_NULLPTR):
		QGraphicsRectItem(x, y, w, h, parent), picker(_picker) {
		setCursor(Qt::CrossCursor);
		setFlag(QGraphicsItem::ItemIsMovable);
		setFlag(QGraphicsItem::ItemIsSelectable);
		setFlag(QGraphicsItem::ItemSendsScenePositionChanges);

	}
	virtual ~AlignRect() {}

protected:
	AlignPicking *picker;
	QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};


class AlignPicking: public ImageViewer {
	Q_OBJECT
public:
	int marker_side = 40;
	Align *align = nullptr;

	AlignRect *rect = nullptr;

	AlignPicking(QWidget *parent = nullptr);
	void setAlign(Align *sphere);
	void updateAlign();
	void clear();

public slots:
	void click(QPoint);
	void updateAlignPoint();
};

#endif
