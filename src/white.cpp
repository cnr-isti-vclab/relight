#include "white.h"

#include <QJsonObject>
#include <QJsonArray>

QJsonObject White::toJson() {
	QJsonObject white;

	QJsonObject jrec;
	jrec.insert("left", rect.left());
	jrec.insert("top", rect.top());
	jrec.insert("width", rect.width());
	jrec.insert("height", rect.height());
	white["rect"] = jrec;


	white.insert("red", red);
	white.insert("green", green);
	white.insert("blue", blue);

	return white;
}

void White::fromJson(QJsonObject obj) {


	auto jrect = obj["rect"].toObject();
	rect.setLeft(jrect["left"].toInt());
	rect.setTop(jrect["top"].toInt());
	rect.setWidth(jrect["width"].toInt());
	rect.setHeight(jrect["height"].toInt());

	red = obj["red"].toDouble();
	green = obj["green"].toDouble();
	blue = obj["blue"].toDouble();
}



