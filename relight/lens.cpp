#include "lens.h"
#include "../src/exif.h"

#include <QJsonObject>

QJsonObject Lens::toJson() {
	QJsonObject lens;
	lens.insert("focal35equivalent", focal35equivalent);
	lens.insert("focalLength", focalLength);
	lens.insert("pixelSizeX", pixelSizeX);
	lens.insert("pixelSizeY", pixelSizeY);
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
	focalLength    = obj["focalLength"].toDouble();
	pixelSizeX  = obj["pixelSizeX"].toDouble();
	pixelSizeY = obj["pixelSizeY"].toDouble();
	principalOffsetX = obj["principalOffsetX"].toDouble();
	principalOffsetY = obj["principalOffsetY"].toDouble();
	k1 = obj["k1"].toDouble();
	k2 = obj["k2"].toDouble();
	p1 = obj["p1"].toDouble();
	p2 = obj["p2"].toDouble();
}

void Lens::readExif(Exif &exif) {
	focalLength = exif[Exif::FocalLength].toDouble();
	//pixelSizeX = exif[Exif::PixelXDimension].toDouble();
	//pixelSizeY = exif[Exif::PixelYDimension].toDouble();

	/*if(focalLength && pixelSizeX && pixelSizeY) {
		focal35equivalent = false;
		return;
	}*/

	focalLength = exif[Exif::FocalLengthIn35mmFilm].toDouble();
	if(focalLength) {
		focal35equivalent = true;
		pixelSizeX = pixelSizeY = 35/(double)width;
	}
	//FocalPlaneXResolution
	//FocalPlaneYResolution
	//FocalPlaneResolutionUnit
	//FocalLength


}
