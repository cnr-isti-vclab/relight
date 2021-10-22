#ifndef QALIGNMARKER_H
#define QALIGNMARKER_H

#include "qmarker.h"

class Align;
class QGraphicsRectItem;

class QAlignMarker: public QMarker {
public:
	Align *align = nullptr;


	explicit QAlignMarker(Align *m, QGraphicsView *_view, QWidget *parent = nullptr);
	~QAlignMarker();

	virtual void click(QPointF pos);

	virtual void setSelected(bool value = true);

public slots:
	virtual void onEdit();

protected:
	QGraphicsRectItem *rect = nullptr;
};

#endif // QALIGNMARKER_H
