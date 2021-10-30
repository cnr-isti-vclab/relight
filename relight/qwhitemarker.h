#ifndef QWHITEMARKER_H
#define QWHITEMARKER_H

#include "qmarker.h"

class White;
class QGraphicsRectItem;

class WhiteMarker: public Marker {
public:
	White *white = nullptr;


	explicit WhiteMarker(White *m, QGraphicsView *_view, QWidget *parent = nullptr);
	~WhiteMarker();

	virtual void click(QPointF pos);

	virtual void setSelected(bool value = true);

public slots:
	virtual void onEdit();

protected:
	QGraphicsRectItem *rect = nullptr;
};


#endif // QWHITEMARKER_H
