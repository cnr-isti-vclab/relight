#ifndef MEASURE_H
#define MEASURE_H



#include <QPointF>

class QJsonObject;
class QPainterPath;
class QGraphicsPathItem;
class QGraphicsLineItem;
class QGraphicsTextItem;
class QGraphicsScene;


class Measure {
public:
	//waiting for first point, waiting for second point, all done.
	enum Measuring { FIRST_POINT = 0, SECOND_POINT = 1, DONE = 2 };
	Measuring measuring = FIRST_POINT;

	QGraphicsPathItem *first = nullptr;
	QGraphicsPathItem *second = nullptr;
	QGraphicsLineItem *line = nullptr;
	QGraphicsTextItem *text = nullptr;
	double length = 0.0;

	Measure();
	~Measure();

	QJsonObject toJson();
	void fromJson(QJsonObject obj);

	void set(QPointF p1, QPointF p2, double measure);
	void setVisible(bool visible);
	void setScene(QGraphicsScene *scene);
	void setFirstPoint(QPointF p);
	void setSecondPoint(QPointF p);
	void setLength(double d);
};

#endif // MEASURE_H
