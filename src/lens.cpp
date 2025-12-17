#include "lens.h"
#include "../src/exif.h"

#include <Eigen/Dense>
#include <QJsonObject>

using namespace Eigen;

double Lens::focal35() {
	if(focal35equivalent) return focalLength;
	else {
		double w = pixelSizeX * width;
		double h = pixelSizeY * height;
		double diag = sqrt(w*w + h*h);
		return focalLength * diag / 43.27;
	}
}

//return vector from eye to pixel (z < 0)
Vector3f Lens::viewDirection(float x, float y) {
	if(!focalLength)
		return Vector3f(0, 0, -1);
	float focal = focalLength;
	if(focal35equivalent) { //focal length assume a diagonal of 43.27
		double w = pixelSizeX * width;
		double h = pixelSizeY * height;
		double diag = sqrt(w*w + h*h);
		focal  = focalLength * diag / 43.27;
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
	focal35equivalent = true;

	
	// Use 35mm equivalent from EXIF if available
	double focalLength35 = exif[Exif::FocalLengthIn35mmFilm].toDouble();
	if(focalLength35) {
		focalLength = focalLength35;
		double diag = sqrt((double)width * width + (double)height * height);
		pixelSizeX = pixelSizeY = 43.27 / diag;
		return;
	}

	focalLength = exif[Exif::FocalLength].toDouble();
	if(!focalLength)
		return;
	
	double focalPlaneXRes = exif[Exif::FocalPlaneXResolution].toDouble();
	double focalPlaneYRes = exif[Exif::FocalPlaneYResolution].toDouble();
	double focalPlaneResUnit = exif[Exif::FocalPlaneResolutionUnit].toDouble();
	
	// Convert resolution unit to mm
	double unitToMm = 25.4; // inches to mm
	if(focalPlaneResUnit == 3)
		unitToMm = 10.0; // cm to mm
	
	// If we have focal plane resolution, calculate actual pixel size
	if(focalPlaneXRes > 0 && focalPlaneYRes > 0 && focalLength > 0) {
		pixelSizeX = unitToMm / focalPlaneXRes;
		pixelSizeY = unitToMm / focalPlaneYRes;
		
		// Calculate 35mm equivalent focal length for reference
		double sensorWidth = pixelSizeX * width;
		double sensorHeight = pixelSizeY * height;
		double sensorDiag = sqrt(sensorWidth * sensorWidth + sensorHeight * sensorHeight);
		focalLength *= 43.27 / sensorDiag;

		double diag = sqrt((double)width * width + (double)height * height);
		pixelSizeX = pixelSizeY = 43.27 / diag;
		
	} else {
		// Last resort: assume full frame sensor
		double diag = sqrt((double)width * width + (double)height * height);
		pixelSizeX = pixelSizeY = 43.27 / diag;
	}
}
