#include "measure.h"

#include <QJsonObject>
#include <QJsonArray>




QJsonObject Measure::toJson() {
	QJsonObject obj;
	obj.insert("unit", "mm");
	obj.insert("length", length);
	QJsonArray jfirst = { first.x(), first.y() };
	obj.insert("first", jfirst);
	QJsonArray jsecond = { second.x(), second.y() };
	obj.insert("second", jsecond);
	return obj;
}

void Measure::fromJson(QJsonObject obj) {
	QString junit = obj["unit"].toString();
	if(junit != "mm") {
		throw QString("Unsupported unit: " + junit);
	}
	length = obj["length"].toDouble();
	QJsonArray jfirst = obj["first"].toArray();
	QJsonArray jsecond = obj["second"].toArray();

	first.setX(jfirst[0].toDouble());
	first.setY(jfirst[1].toDouble());

	second.setX(jsecond[0].toDouble());
	second.setY(jsecond[1].toDouble());
}

