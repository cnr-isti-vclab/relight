#include "align.h"

#include <QJsonObject>
#include <QJsonArray>

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
	rect.setLeft(jrect["left"].toDouble());
	rect.setTop(jrect["top"].toDouble());
	rect.setWidth(jrect["width"].toDouble());
	rect.setHeight(jrect["height"].toDouble());

	offsets.clear();
	for(auto joffsets: obj["offsets"].toArray()) {
		auto j = joffsets.toArray();
		offsets.push_back(QPointF(j[0].toDouble(), j[1].toDouble()));
	}
}

void Align::readThumb(QImage img, int n) {
	if(n == 0)
		thumbs.clear();
	thumbs.push_back(img.copy(rect.toRect()));
}
