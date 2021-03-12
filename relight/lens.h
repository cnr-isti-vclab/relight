#ifndef LENS_H
#define LENS_H

#include <vector>

class QJsonObject;


class Lens {
public:
	int width = 0, height = 0;
	bool focal35equivalent = true;
	double focalx = 0, focaly = 0; //in mm
	double ccdWidth = 0;           //in mm
	double ccdHeight = 0;          //in mm
	double principalOffsetX = 0, principalOffsetY = 0;
	double k1 = 0, k2 = 0, p1 = 0, p2 = 0;

	QJsonObject toJson();
	void fromJson(const QJsonObject &obj);

	double focalX() {
		if(focal35equivalent)
			return 35*focalx/ccdWidth;
		return focalx;
	}

	double focalY() {
		if(focal35equivalent)
			return 35*focaly/ccdHeight;
		return focaly;
	}

	double focal35X() {
		if(focal35equivalent)
			return focalx;
		return ccdWidth*focalx/35;
	}


	double focal35Y() {
		if(focal35equivalent)
			return focaly;
		return ccdHeight*focaly/35;
	}

	std::vector<double> matrix() {
		std::vector<double> m = { focalX(), 0, principalOffsetX };
		return m;
	}
};

#endif // LENS_H
