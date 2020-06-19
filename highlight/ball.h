#ifndef BALL_H
#define BALL_H

#include <QPoint>
#include <QSize>
#include <QRect>
#include <QImage>
#include <QGraphicsEllipseItem>
#include <QRunnable>
#include <vector>

#include "../src/vector.h"

class BorderPoint: public QGraphicsEllipseItem {
public:
	BorderPoint(qreal x, qreal y, qreal w, qreal h, QGraphicsItem *parent = Q_NULLPTR):
		QGraphicsEllipseItem(x, y, w, h, parent) {}
	virtual ~BorderPoint();

protected:
	QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};

class HighlightPoint: public QGraphicsEllipseItem {
public:
	HighlightPoint(qreal x, qreal y, qreal w, qreal h, QGraphicsItem *parent = Q_NULLPTR):
		QGraphicsEllipseItem(x, y, w, h, parent) {}
	virtual ~HighlightPoint();

protected:
	QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};



class Ball {
public:
	Ball(int n_lights) {
		lights.resize(n_lights);
		directions.resize(n_lights);
		positions.resize(n_lights);
		valid.resize(n_lights, false);
	}
	void run();

	bool active = false;
	bool only_directions = false;
	QPointF center;      //in pixel coordinates of the image
	float radius;        //fitted radius
	float smallradius;   //innner radius where to look for reflections
	QRect inner;         //box of the inner part of the sphere.
	bool fitted;         //we have a valid fit

	std::vector<QPointF> lights;
	std::vector<Vector3f> directions;
	std::vector<Vector3f> positions;  //3d positions if using more than 1 ball.

	std::vector<bool> valid;
	std::vector<BorderPoint *> border;

	QGraphicsEllipseItem *circle = nullptr;
	HighlightPoint *highlight = nullptr;
	QImage sphere;

	Ball();
	bool fit(QSize imgsize);
	void findHighlight(QImage im, int n);
	void computeDirections();
	void save(QString filename, QStringList images);

	void setActive(bool active);
};

#endif // BALL_H
