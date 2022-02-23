#ifndef MEASURE_H
#define MEASURE_H



#include <QPointF>

class QJsonObject;


class Measure {
public:
	QPointF first, second;
	double length = 0.0;


	QJsonObject toJson();
	void fromJson(QJsonObject obj);

	void set(QPointF p1, QPointF p2, double measure);
	void setFirstPoint(QPointF p);
	void setSecondPoint(QPointF p);
	void setLength(double d);
};

#endif // MEASURE_H
