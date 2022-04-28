#ifndef QSPHEREMARKER_H
#define QSPHEREMARKER_H

#include "qmarker.h"

#include <QGraphicsEllipseItem>

class Sphere;


class BorderPoint: public QGraphicsEllipseItem {
public:
	BorderPoint(qreal x, qreal y, qreal w, qreal h, QGraphicsItem *parent = Q_NULLPTR):
		QGraphicsEllipseItem(x, y, w, h, parent) {}
	virtual ~BorderPoint();

protected:
	QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};

/*class HighlightPoint: public QGraphicsEllipseItem {
public:
	HighlightPoint(qreal x, qreal y, qreal w, qreal h, QGraphicsItem *parent = Q_NULLPTR):
		QGraphicsEllipseItem(x, y, w, h, parent) {}
	virtual ~HighlightPoint();

protected:
	QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};*/

class HighlightPoint: public QGraphicsPathItem {
public:
	HighlightPoint(qreal x, qreal y, qreal w, qreal h, QGraphicsItem *parent = Q_NULLPTR):
		QGraphicsPathItem(parent) {
		QPainterPath path;
		path.moveTo(x, y);
		path.lineTo(w, h);
		path.moveTo(x, h);
		path.lineTo(w, y);
		setPath(path);
	}
	virtual ~HighlightPoint();

protected:
	QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};



class SphereMarker: public Marker {
	Q_OBJECT
public:
	Sphere *sphere = nullptr;

	std::vector<BorderPoint *> border;
	QGraphicsEllipseItem *circle = nullptr;
	QGraphicsEllipseItem *smallcircle = nullptr;
	HighlightPoint *highlight = nullptr;
	QGraphicsPixmapItem *pixmap = nullptr;


	explicit SphereMarker(Sphere *sphere, QGraphicsView *_view, QWidget *parent = nullptr);
	~SphereMarker();

	void showHighlight(size_t n);
	void updateHighlightPosition(size_t n);

	void addBorderPoint(QPointF pos);
	void updateBorderPoint(QGraphicsEllipseItem *point);

	void deleteSelected(int currentImage);
	void fit();
	virtual void click(QPointF pos);
	virtual void doubleClick(QPointF pos) { click(pos); }

	virtual void cancelEditing() {}

	virtual void setSelected(bool value);

private:
	void init(); //set graphicsitem from sphere.

};


#endif // QSPHEREMARKER_H
