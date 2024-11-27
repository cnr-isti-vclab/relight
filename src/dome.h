#ifndef DOME_H
#define DOME_H

#include "relight_vector.h"

#include <vector>
#include <Eigen/Core>
#include <QString>

class QJsonObject;
class Lens;
class Sphere;


/* Dome specify the geometry for the lights.
 * Three cases:
 * 1. Directional: lights are considered as infinite distance directional lights
 * 2. Spherical: the 3d position of the lights is computed starting from the directions and assuming
 *               they live on a sphere.
 * 3. Lights3D: positions only are used, the sphere parameters, (width diameter, offset) are ignored.
 *
 * Coordinate system for the positions.
 * The origin is the center of the image. Z is up.
 * All units are in mm, but for the RTI computations what matters is only the ratio to the image width
 * If specified, the width will be used to display the scale in viewers.
 */

class Dome {
public:
	QString label;
	QString notes;
	std::vector<Eigen::Vector3f> directions;       //estimated infinite far away lights.
	std::vector<Eigen::Vector3f> positionsSphere;  //3d positions if sphericals
	std::vector<Eigen::Vector3f> positions3d;      //3d positions

	//TODO rename
	std::vector<Color3f> ledAdjust;  //multiply pixel valut to correct for led differences
	//TODO dome calibrations as a grid per led

	enum LightConfiguration { DIRECTIONAL, SPHERICAL, LIGHTS3D };
	LightConfiguration lightConfiguration = DIRECTIONAL;

	double imageWidth = 0.0; //in mm
	double domeDiameter = 0.0;
	double verticalOffset = 0.0;

	Dome();
	Dome(const QString &filename) { load(filename); }
	void fromSpheres(std::vector<Sphere *> &spheres, Lens &lens);
	void load(const QString &filename);
	void save(const QString &filename);
	size_t imagesCount() { return directions.size(); }

	QJsonObject toJson();
	void fromJson(const QJsonObject &obj);
};

#endif // DOME_H
