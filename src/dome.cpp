#include "dome.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QList>
#include <QFile>

using namespace std;
Dome::Dome() {}


static QStringList lightConfigs = { "directional", "spherical", "lights3d" };

QJsonArray toJson(std::vector<Vector3f> &values) {
	QJsonArray jvalues;
	for(Vector3f v: values) {
		QJsonObject obj;
		obj["x"] = v[0];
		obj["y"] = v[1];
		obj["z"] = v[2];
		jvalues.append(obj);
	}
	return jvalues;
}

QJsonArray toJson(std::vector<Color3f> &values) {
	QJsonArray jvalues;
	for(Color3f v: values) {
		QJsonObject obj;
		obj["r"] = v[0];
		obj["g"] = v[1];
		obj["b"] = v[2];
		jvalues.append(obj);
	}
	return jvalues;
}

void Dome::save(const QString &filename) {
	QFile file(filename);
	bool open = file.open(QFile::WriteOnly | QFile::Truncate);
	if(!open) {
		throw file.errorString();
	}
	QJsonDocument doc(toJson());
	file.write(doc.toJson());
}

void Dome::load(const QString &filename) {
	QFile file(filename);
	bool open = file.open(QFile::ReadOnly);
	if(!open) {
		throw file.errorString();
	}
	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	fromJson(doc.object());
}


QJsonObject Dome::toJson() {
	QJsonObject dome;
	dome.insert("label", label);
	dome.insert("notes", notes);
	dome.insert("imageWidth", imageWidth);
	dome.insert("domeDiameter", domeDiameter);
	dome.insert("verticalOffset", verticalOffset);
	dome.insert("lightConfiguration", lightConfigs[lightConfiguration]);
	dome.insert("directions", ::toJson(directions));
	dome.insert("positions", ::toJson(positions));
	dome.insert("ledAdjust", ::toJson(ledAdjust));
	return dome;
}

 void fromJson(QJsonArray values, std::vector<Vector3f> &lights) {
	lights.clear();
	for(auto v: values) {
		auto o = v.toObject();
		lights.push_back(Vector3f(o["x"].toDouble(), o["y"].toDouble(), o["z"].toDouble()));
	}
}

void fromJson(QJsonArray values, std::vector<Color3f> &lights) {
	lights.clear();
	for(auto v: values) {
		auto o = v.toObject();
		lights.push_back(Color3f(o["r"].toDouble(), o["g"].toDouble(), o["b"].toDouble()));
	}
}


void Dome::fromJson(const QJsonObject &obj) {
	label = obj["label"].toString();
	notes = obj["notes"].toString();
	imageWidth     = obj["imageWidth"].toDouble();
	domeDiameter   = obj["domeDiameter"].toDouble();
	verticalOffset = obj["verticalOffset"].toDouble();
	if(obj.contains("lightConfiguration")) {
		lightConfiguration = DIRECTIONAL;
		int index = lightConfigs.indexOf(obj["lightConfiguration"].toString());
		if(index >= 0)
			lightConfiguration = LightConfiguration(index);
	}
	::fromJson(obj["directions"].toArray(), directions);
	::fromJson(obj["positions"].toArray(), positions);
	::fromJson(obj["ledAdjust"].toArray(), ledAdjust);
}

