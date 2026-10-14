#include "align/align.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QPainter>

#include <algorithm>
#include <assert.h>

QJsonObject Align::toJson() {
	QJsonObject align;

	QJsonObject jrec;
	jrec.insert("left", rect.left());
	jrec.insert("top", rect.top());
	jrec.insert("width", rect.width());
	jrec.insert("height", rect.height());
	align["rect"] = jrec;

	QJsonArray joffsets;
	for(QPointF l: offsets) {
		QJsonArray joffset = { l.x(), l.y() };
		joffsets.append(joffset);
	}
	align["offsets"] = joffsets;

	return align;
}

void Align::fromJson(QJsonObject obj) {


	auto jrect = obj["rect"].toObject();
	rect.setLeft(jrect["left"].toInt());
	rect.setTop(jrect["top"].toInt());
	rect.setWidth(jrect["width"].toInt());
	rect.setHeight(jrect["height"].toInt());

	offsets.clear();
	for(auto joffsets: obj["offsets"].toArray()) {
		auto j = joffsets.toArray();
		offsets.push_back(QPointF(j[0].toDouble(), j[1].toDouble()));
	}
	thumbs.resize(offsets.size());
}

QString Align::cacheFilename() const {
	return QString("resources/align_%1x%2+%3+%4.jpg")
		.arg(rect.width())
		.arg(rect.height())
		.arg(rect.left())
		.arg(rect.top());
}

void Align::readThumb(QImage tile, int n) {
	int x = (rect.width() - thumbnail_side)/2;
	int y = (rect.height() - thumbnail_side)/2;
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
