#ifndef QWHITEMARKER_H
#define QWHITEMARKER_H

#include "qmarker.h"

class White;
class QGraphicsRectItem;

class QWhiteMarker: public QMarker {
public:
	White *white = nullptr;


	explicit QWhiteMarker(White *m, QGraphicsView *_view, QWidget *parent = nullptr);
	~QWhiteMarker();

	virtual void click(QPointF pos);

	virtual void setSelected(bool value = true);

public slots:
	virtual void onEdit();

protected:
	QGraphicsRectItem *rect = nullptr;
};


#endif // QWHITEMARKER_H
