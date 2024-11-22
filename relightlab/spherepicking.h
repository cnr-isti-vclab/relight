#ifndef SPHERE_PICKING_H
#define SPHERE_PICKING_H

#include "imageview.h"

#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsScene>

class Canvas;
class QGraphicsEllipseItem;
class QGraphicsLineItem;
class SpherePicking;

class BorderPoint: public QGraphicsEllipseItem {
public:
	BorderPoint(SpherePicking *_picker, qreal x, qreal y, qreal w, qreal h, QGraphicsItem *parent = Q_NULLPTR):
		QGraphicsEllipseItem(x, y, w, h, parent), picker(_picker) {
		setCursor(Qt::CrossCursor);
		setFlag(QGraphicsItem::ItemIsMovable);
		setFlag(QGraphicsItem::ItemIsSelectable);
		setFlag(QGraphicsItem::ItemSendsScenePositionChanges);

	}
	virtual ~BorderPoint() {}

protected:
	SpherePicking *picker;
	QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};

class HighlightPoint: public QGraphicsPathItem {
public:
	HighlightPoint(SpherePicking *_picker, qreal x, qreal y, qreal w, qreal h, QGraphicsItem *parent = Q_NULLPTR):
		QGraphicsPathItem(parent), picker(_picker) {
		QPainterPath path;
		path.moveTo(x, y);
		path.lineTo(w, h);
		path.moveTo(x, h);
		path.lineTo(w, y);
		setPath(path);
	}
	virtual ~HighlightPoint();

protected:
	SpherePicking *picker;
	QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};

class Sphere;

class SpherePicking: public ImageViewer {
	Q_OBJECT
public:
	Sphere *sphere = nullptr;

	std::vector<BorderPoint *> border;
	QGraphicsEllipseItem *circle = nullptr;
	QGraphicsEllipseItem *smallcircle = nullptr;
	QGraphicsLineItem *axis[2] = { nullptr, nullptr };
	HighlightPoint *highlight = nullptr;
	QGraphicsPixmapItem *pixmap = nullptr;


	SpherePicking(QWidget *parent = nullptr);
	void setSphere(Sphere *sphere);
	void updateSphere();
	void clear();

	void showHighlight(size_t n);
	void updateHighlightPosition();

	void addBorderPoint(QPointF pos);
	void updateBorderPoints();

	void deleteSelected(int currentImage);
	void fitSphere();

/*	virtual void doubleClick(QPointF pos) { click(pos); }

	virtual void cancelEditing() {}

	virtual void setSelected(bool value); */


public slots:
	void click(QPoint);
	//void cancel();
	//void accept();
	void showImage(int id);

protected:
	void keyReleaseEvent(QKeyEvent *e);
};

#endif
