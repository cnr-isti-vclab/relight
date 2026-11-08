#include "align/align.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QPainter>

#include <algorithm>
#include <assert.h>

QJsonObject Align::toJson() {
	QJsonObject align;
	QJsonArray jregions;
	for(const QRect &region: regions) {
		QJsonArray jregion = { region.left(), region.top(), region.width(), region.height() };
		jregions.append(jregion);
	}
	align["regions"] = jregions;

	QJsonArray jcorrespondences;
	for(const AutoAlignCorrespondence &correspondence: correspondences) {
		QJsonObject jcorrespondence;
		jcorrespondence["first"] = correspondence.first;
		jcorrespondence["second"] = correspondence.second;
		jcorrespondence["position"] = QJsonArray{
			correspondence.position.x(), correspondence.position.y()};
		jcorrespondence["offset"] = QJsonArray{
			correspondence.peak.offset.x(), correspondence.peak.offset.y()};
		jcorrespondence["response"] = correspondence.peak.response;
		jcorrespondences.append(jcorrespondence);
	}
	align["correspondences"] = jcorrespondences;

	return align;
}

void Align::fromJson(QJsonObject obj) {
	regions.clear();
	for(const QJsonValue &value: obj["regions"].toArray()) {
		const QJsonArray region = value.toArray();
		if(region.size() != 4)
			continue;
		regions.emplace_back(region[0].toInt(), region[1].toInt(),
			region[2].toInt(), region[3].toInt());
	}
	// Migrate projects that stored one manually selected rectangle.
	if(regions.empty() && obj.contains("rect")) {
		const QJsonObject legacy_rect = obj["rect"].toObject();
		regions.emplace_back(legacy_rect["left"].toInt(), legacy_rect["top"].toInt(),
			legacy_rect["width"].toInt(), legacy_rect["height"].toInt());
	}
	correspondences.clear();
	for(const QJsonValue &value: obj["correspondences"].toArray()) {
		const QJsonObject jcorrespondence = value.toObject();
		const QJsonArray position = jcorrespondence["position"].toArray();
		const QJsonArray offset = jcorrespondence["offset"].toArray();
		if(position.size() != 2 || offset.size() != 2)
			continue;
		AutoAlignCorrespondence correspondence;
		correspondence.first = jcorrespondence["first"].toInt(-1);
		correspondence.second = jcorrespondence["second"].toInt(-1);
		correspondence.position = Eigen::Vector2f(
			float(position[0].toDouble()), float(position[1].toDouble()));
		correspondence.peak.offset = Eigen::Vector2f(
			float(offset[0].toDouble()), float(offset[1].toDouble()));
		correspondence.peak.response = float(jcorrespondence["response"].toDouble());
		correspondences.push_back(correspondence);
	}
}

QString Align::cacheFilename() const {
	assert(!regions.empty());
	const QRect &region = regions.front();
	return QString("resources/align_%1x%2+%3+%4.jpg")
		.arg(region.width())
		.arg(region.height())
		.arg(region.left())
		.arg(region.top());
}

void Align::readThumb(QImage tile, int n) {
	assert(!regions.empty());
	int x = (regions.front().width() - thumbnail_side)/2;
	int y = (regions.front().height() - thumbnail_side)/2;
	thumbs[size_t(n)] = tile.copy(x, y, thumbnail_side, thumbnail_side);
}

bool Align::readCacheThumbs(QImage img) {
	const int count = int(thumbs.size());
	const int new_width = 20*thumbnail_side;
	const int new_height = std::max(1, (count + 19)/20)*thumbnail_side;

	if(img.size() != QSize(new_width, new_height))
		return false;

	for(int i = 0; i < count; i++) {
		int x = (i%20)*thumbnail_side;
		int y = (i/20)*thumbnail_side;
		thumbs[size_t(i)] = img.copy(x, y, thumbnail_side, thumbnail_side);
	}
	return true;
}

void Align::saveCacheThumbs(QString filename) {
	int w = 20*thumbnail_side;
	int h = std::max(1, (int(thumbs.size()) + 19)/20)*thumbnail_side;
	QImage img(w, h, QImage::Format_RGB32);
	img.fill(0);
	QPainter painter(&img);
	for(size_t i = 0; i < thumbs.size(); i++) {
		if(thumbs[i].isNull())
			continue;
		int x = (int(i)%20)*thumbnail_side;
		int y = (int(i)/20)*thumbnail_side;
		painter.drawImage(x, y, thumbs[i]);
	}
	img.save(filename, "jpg", 95);
}
