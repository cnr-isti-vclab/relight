#ifndef MEASURE_H
#define MEASURE_H



#include <QPointF>
#include <QLineF>

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
	bool isValid() {
		if(first.isNull() || second.isNull())
			return false;
		if(QLineF(first, second ).length() ==0)
			return false;
		return length > 0;
	}
	float pixelSize() {
		return length/QLineF(first, second ).length();
	}
};

#endif // MEASURE_H
