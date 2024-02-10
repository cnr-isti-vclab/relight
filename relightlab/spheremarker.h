#ifndef SPHEREPICKING_H
#define SPHEREPICKING_H

#include <QDialog>
#include <QGraphicsScene>

class QGraphicsView;
/*
class SpherePicking: public QDialog {
public:
	SpherePicking(QWidget *parent = nullptr);

public slots:
	void updatePoint(QGraphicsEllipseItem *point);
private:
	QGraphicsView *view;
	QGraphicsScene scene;
};*/

#ifndef SPHEREMARKER_H
#define SPHEREMARKER_H

#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>

class Sphere;
class QGraphicsView;

class BorderPoint: public QGraphicsEllipseItem {
public:
	BorderPoint(qreal x, qreal y, qreal w, qreal h, QGraphicsItem *parent = Q_NULLPTR):
		QGraphicsEllipseItem(x, y, w, h, parent) {
		setCursor(Qt::CrossCursor);
		setFlag(QGraphicsItem::ItemIsMovable);
		setFlag(QGraphicsItem::ItemIsSelectable);
		setFlag(QGraphicsItem::ItemSendsScenePositionChanges);

	}
	virtual ~BorderPoint();

protected:
	QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};

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

class SphereScene: public QGraphicsScene {
	Q_OBJECT
public:
	SphereScene(QObject *parent = nullptr): QGraphicsScene(parent) {}

signals:
	void pointsChanged();
	void highlightMoved();
};


class SphereMarker {
public:
	Sphere *sphere = nullptr;

	std::vector<BorderPoint *> border;
	QGraphicsEllipseItem *circle = nullptr;
	QGraphicsEllipseItem *smallcircle = nullptr;
	QGraphicsLineItem *axis[2] = { nullptr, nullptr };
	HighlightPoint *highlight = nullptr;
	QGraphicsPixmapItem *pixmap = nullptr;


	explicit SphereMarker(Sphere *sphere, QGraphicsView *_view);
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
#endif // SPHEREMARKER_H



#endif // SPHEREPICKING_H
