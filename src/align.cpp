#include "align.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QPainter>

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

void Align::readThumb(QImage img, int n) {
	thumbs[n] = img.copy(rect);
}

void Align::readCacheThumbs(QImage img) {
	int w = 20*rect.width();
	int h = (1 + thumbs.size()/20)*rect.height();
	assert(img.width() == w);
	assert(img.height() == h);

	for(int i = 0; i < thumbs.size(); i++) {
		int x = (i%20)*rect.width();
		int y = (i/20)*rect.height();
		thumbs[i] = img.copy(x, y, rect.width(), rect.height());
	}
}

void Align::saveCacheThumbs(QString filename) {
	//set quality 95

	int w = 20*rect.width();
	int h = (1 + thumbs.size()/20)*rect.height();
	QImage img(w, h, QImage::Format_ARGB32);
	img.fill(0);
	for(int i = 0; i < thumbs.size(); i++) {
		if(thumbs[i].isNull())
			continue;
		int x = (i%20)*rect.width();
		int y = (i/20)*rect.height();
		QPainter painter(&img);
		painter.drawImage(x, y, thumbs[i]);
	}
	img.save(filename, "jpg", 95);
}
