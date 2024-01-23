#ifndef WHITE_H
#define WHITE_H


#include <QPoint>
#include <QRect>
#include <vector>

class QJsonObject;

class White {
public:
	QRectF rect;
	double red = 1.0;
	double green = 1.0;
	double blue = 1.0;

	White() {}
	QJsonObject toJson();
	void fromJson(QJsonObject obj);
};
#endif // WHITE_H
