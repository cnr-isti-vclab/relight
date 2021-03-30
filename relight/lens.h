#ifndef LENS_H
#define LENS_H

#include <vector>

class QJsonObject;
class Exif;

class Lens {
public:
	int width = 0, height = 0;
	bool focal35equivalent = true;
	double focalLength = 0; //in mm
	double pixelSizeX = 0, pixelSizeY = 0;
	double principalOffsetX = 0, principalOffsetY = 0;

	//double ccdWidth = 0;           //in mm
	//double ccdHeight = 0;          //in mm

	double k1 = 0, k2 = 0, p1 = 0, p2 = 0;

	QJsonObject toJson();
	void fromJson(const QJsonObject &obj);
	void readExif(Exif &exif);

	//compute focal length 35mm equivalent.
	double focal35() {
		if(focal35equivalent) return focalLength;
		else {
			double w = pixelSizeX * width;
			return focalLength * 35 / w;
		}
	}

	double ccdWidth() {
		return pixelSizeX*width;
	}

	double ccdHeight() {
		return pixelSizeY*height;
	}

	std::vector<double> matrix() {
		double fx = focalLength /pixelSizeX;
		double fy = focalLength /pixelSizeY;
		double ox = principalOffsetX;
		double oy = principalOffsetY;
		std::vector<double> m = {
			-fx, 0, ox,
			-fy, 0, oy,
			0, 0, 1
		};
		return m;
	}
};

#endif // LENS_H
