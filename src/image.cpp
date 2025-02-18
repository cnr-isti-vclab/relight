#include "image.h"
#include "../src/exif.h"

#include <QJsonArray>
#include <QJsonObject>

#include <iostream>
using namespace std;
using namespace Eigen;

QJsonObject toJson(const Vector3f &v) {
	QJsonObject jv;
	jv.insert("x", v[0]);
	jv.insert("y", v[1]);
	jv.insert("z", v[2]);
	return jv;
}

void fromJson(const QJsonObject &obj, Vector3f &v) {
	v[0] = obj["x"].toDouble();
	v[1] = obj["y"].toDouble();
	v[2] = obj["z"].toDouble();
}

QJsonObject Image::toJson() {

	QJsonObject obj;
	obj.insert("filename", filename);
	obj.insert("skip", skip);
	obj.insert("visible", visible);
	return obj;
}
void Image::fromJson(const QJsonObject &obj) {
	filename = obj["filename"].toString();
	skip = obj["skip"].toBool(false);
	visible = obj["visible"].toBool(true);
}

void Image::readExif(Exif &exif) {
	exposureTime = exif[Exif::ExposureTime].toDouble();
	isoSpeedRatings = exif[Exif::ISOSpeedRatings].toDouble();
	//ColorSpace

	/*for(auto tag: exif.keys()) {
		cout << qPrintable(exif.tagNames[tag]) << " = " << qPrintable(exif[tag].toString()) << endl;
	}*/
}
