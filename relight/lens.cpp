#include "lens.h"

#include <QJsonObject>

QJsonObject Lens::toJson() {
	QJsonObject lens;
	lens.insert("focal35equivalent", focal35equivalent);
	lens.insert("focalx", focalx);
	lens.insert("focaly", focaly);
	lens.insert("ccdWidth", ccdWidth);
	lens.insert("ccdHeight", ccdHeight);
	lens.insert("principalOffsetX", principalOffsetX);
	lens.insert("principalOffsetY", principalOffsetY);
	lens.insert("k1", k1);
	lens.insert("k2", k2);
	lens.insert("p1", p1);
	lens.insert("p2", p2);
	return lens;
}

void Lens::fromJson(const QJsonObject &obj) {
	focal35equivalent  = obj["focal35equivalent"].toBool();
	focalx    = obj["focalx"].toDouble();
	focaly    = obj["focaly"].toDouble();
	ccdWidth  = obj["ccdWidth"].toDouble();
	ccdHeight = obj["ccdHeight"].toDouble();
	principalOffsetX = obj["principalOffsetX"].toDouble();
	principalOffsetY = obj["principalOffsetY"].toDouble();
	k1 = obj["k1"].toDouble();
	k2 = obj["k2"].toDouble();
	p1 = obj["p1"].toDouble();
	p2 = obj["p2"].toDouble();
}
