#ifndef QALIGNMARKER_H
#define QALIGNMARKER_H

#include "qmarker.h"
#include <QStringList>
#include <QPointF>

class Align;
class QGraphicsRectItem;

class AlignMarker: public Marker {
Q_OBJECT
public:
	Align *align = nullptr;


	explicit AlignMarker(Align *m, QGraphicsView *_view, QWidget *parent = nullptr);
	virtual ~AlignMarker();

	virtual void click(QPointF pos);

	virtual void setSelected(bool value = true);
	QStringList images;

public slots:
	virtual void onEdit();

signals:
	void showTable(AlignMarker *marker);

protected:
	QGraphicsRectItem *rect = nullptr;
};

#endif // QALIGNMARKER_H
