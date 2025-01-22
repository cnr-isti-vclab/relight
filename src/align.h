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

	QRect rect;
	std::vector<QPointF> offsets; //from the center of the rect
	std::vector<QImage> thumbs;

	QJsonObject toJson();
	void fromJson(QJsonObject obj);
	void readThumb(QImage img, int n);
};

#endif // ALIGN_H
