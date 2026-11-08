#ifndef RELIGHT_ALIGN_H
#define RELIGHT_ALIGN_H

#include <QImage>
#include <QPoint>
#include <QRect>
#include <QString>
#include <vector>

#include "autoalign.h"


class QJsonObject;

class Align {
public:
	static constexpr int thumbnail_side = 40;

	std::vector<QRect> regions;
	std::vector<AutoAlignCorrespondence> correspondences;
	std::vector<QImage> thumbs;

	Align(int n_lights) {
		thumbs.resize(n_lights);
	}

	QJsonObject toJson();
	void fromJson(QJsonObject obj);
	QString cacheFilename() const;
	void readThumb(QImage tile, int n);

	bool readCacheThumbs(QImage img);
	void saveCacheThumbs(QString filename);
};

#endif // RELIGHT_ALIGN_H
