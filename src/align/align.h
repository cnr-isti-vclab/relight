#ifndef RELIGHT_ALIGN_H
#define RELIGHT_ALIGN_H

#include <QImage>
#include <QPoint>
#include <QRect>
#include <QString>
#include <vector>


class QJsonObject;

class Align {
public:
	static constexpr int thumbnail_side = 40;

	Align(int n_lights) {
		offsets.resize(n_lights, QPointF(0, 0));
		thumbs.resize(n_lights);
	}

	QRect rect;
	std::vector<QPointF> offsets; //from the center of the rect, [0,0] is not used.
	std::vector<QImage> thumbs;

	QJsonObject toJson();
	void fromJson(QJsonObject obj);
	QString cacheFilename() const;
	void readThumb(QImage tile, int n);

	bool readCacheThumbs(QImage img);
	void saveCacheThumbs(QString filename);
};

#endif // RELIGHT_ALIGN_H
