#ifndef LENS_H
#define LENS_H

#include <Eigen/Core>
#include <vector>

class QJsonObject;
class Exif;

class Lens {
public:
	int width = 0, height = 0; //in pixels of the image
	bool focal35equivalent = true;
	double focalLength = 0; //in mm
	double pixelSizeX = 0, pixelSizeY = 0; //in mm! this is needed to compute geometric distances using the focal length in mm.
	double principalOffsetX = 0, principalOffsetY = 0;

	//double ccdWidth = 0;           //in mm
	//double ccdHeight = 0;          //in mm

	double k1 = 0, k2 = 0, p1 = 0, p2 = 0;

	QJsonObject toJson();
	void fromJson(const QJsonObject &obj);
	void readExif(Exif &exif);

	//compute focal length 35mm equivalent.
	double focal35();

	//return vector from eye to pixel (z < 0)
	Eigen::Vector3f viewDirection(float x, float y);
	//rotate a normal computed on a plane perpendicular to the view direction on the image plane
	Eigen::Vector3f rotateNormal(Eigen::Vector3f n, float x, float y);

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
