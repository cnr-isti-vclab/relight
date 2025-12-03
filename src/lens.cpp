#include "lens.h"
#include "../src/exif.h"

#include <Eigen/Dense>
#include <QJsonObject>

using namespace Eigen;

double Lens::focal35() {
	if(focal35equivalent) return focalLength;
	else {
		double w = pixelSizeX * width;
		return focalLength * 35 / w;
	}
}

//return vector from eye to pixel (z < 0)
Vector3f Lens::viewDirection(float x, float y) {
	if(!focalLength)
		return Vector3f(0, 0, -1);
	float focal = focalLength;
	if(focal35equivalent) {
		double w = pixelSizeX * width;
		focal  = focalLength * w / 35;
	}
	x -= width/2;
	y -= height/2;
	return Vector3f(x*pixelSizeX, -y*pixelSizeY, -focal);
}
Vector3f Lens::rotateNormal(Vector3f n, float x, float y) {
	Vector3f view = viewDirection(x, y);
	Vector3f center(0.0f, 0.0f, -1.0f);
	Vector3f axis = center.cross(view);

	if (axis.norm() < 0.00001)
		return n;

	Vector3f unit_axis = axis.normalized();

	float cos_theta = view.dot(center) / (view.norm() * center.norm());
	cos_theta = std::min(std::max(cos_theta, -1.0f), 1.0f);
	float theta = acos(cos_theta);

	AngleAxisf rotation(theta, unit_axis);
	return rotation * n;
}


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
