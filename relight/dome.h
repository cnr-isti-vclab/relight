#ifndef DOME_H
#define DOME_H

#include <vector>
#include "../src/vector.h"

class QJsonObject;

//all units are in cm, but for the RTI computations what matters is only the ratio to the image widthl
class Dome {
public:
	std::vector<Vector3f> directions;
	std::vector<Vector3f> positions;
	std::vector<Color3f> ledAdjust;  //multiply pixel valut to correct for led differences

	enum LightConfiguration { DIRECTIONAL, SPHERICAL, LIGHTS3D };
	LightConfiguration lightConfiguration = DIRECTIONAL;

	double imageWidth = 0.0; //in cm
	double domeDiameter = 0.0;
	double verticalOffset = 0.0;

	Dome();
	QJsonObject toJson();
	void fromJson(const QJsonObject &obj);
};

#endif // DOME_H
