#ifndef ALIGN_H
#define ALIGN_H

#include <QImage>
#include <QPoint>
#include <QRect>
#include <vector>


class QJsonObject;

class Align {
public:
	Align(int n_lights) {
		offsets.resize(n_lights);
	}

	QRectF rect;
	std::vector<QPointF> offsets; //from the center of the rect, [0,0] is not used.
	std::vector<QImage> thumbs;

	QJsonObject toJson();
	void fromJson(QJsonObject obj);
	void readThumb(QImage img, int n);
};

#endif // ALIGN_H
