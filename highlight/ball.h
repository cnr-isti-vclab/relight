#ifndef BALL_H
#define BALL_H

#include <QPoint>
#include <QSize>
#include <QRect>
#include <QImage>
#include <QGraphicsEllipseItem>
#include <vector>

class BorderPoint: public QGraphicsEllipseItem {
public:
	BorderPoint(qreal x, qreal y, qreal w, qreal h, QGraphicsItem *parent = Q_NULLPTR):
		QGraphicsEllipseItem(x, y, w, h, parent) {}
	virtual ~BorderPoint();

protected:
	QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};

class Ball {
public:
	Ball(int n_lights) {
		lights.resize(n_lights);
		valid.resize(n_lights, false);
	}

	QPointF center;      //in pixel coordinates of the image
	float radius;        //fitted radius
	float smallradius;   //innner radius where to look for reflections
	QRect inner;         //box of the inner part of the sphere.
	bool fitted;         //we have a valid fit

	std::vector<QPointF> lights;
	std::vector<bool> valid;
	std::vector<BorderPoint *> border;

	QGraphicsEllipseItem *circle = nullptr;
	QGraphicsEllipseItem *highlight = nullptr;
	QImage sphere;

	Ball();
	bool fit(QSize imgsize);
	void findHighlight(QImage im, int n);
};

#endif // BALL_H
