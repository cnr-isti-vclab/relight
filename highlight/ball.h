#ifndef BALL_H
#define BALL_H

#include <QPoint>
#include <QSize>
#include <QRect>

#include <vector>

class Ball {
public:
	QPointF center;      //in pixel coordinates of the image
	float radius;        //fitted radius
	float smallradius;   //innner radius where to look for reflections
	QRect inner;         //box of the inner part of the sphere.

	std::vector<QPointF> lights;

	Ball();
};

#endif // BALL_H
