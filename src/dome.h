#ifndef DOME_H
#define DOME_H

#include "relight_vector.h"

#include <vector>
#include <Eigen/Core>
#include <QString>

class QJsonObject;
class Image;
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
 *
 * Light source tracking (lightSource):
 * - FROM_SPHERES: Lights computed from reflective spheres.
 *   When fromSpheres() is called with geometry parameters, it computes all three arrays:
 *   directions, positionsSphere, and positions3d.
 *   Must recompute (call fromSpheres() again) when:
 *   - Images are skipped/unskipped (changes valid light count and recomputes all arrays)
 *   - Spheres are added/removed/modified (changes light detection/directions)
 *   - Geometry parameters change (imageWidth, domeDiameter, verticalOffset) - updates positions with new geometry
 *   Switching between DIRECTIONAL/SPHERICAL/LIGHTS3D does NOT require recomputation
 *   (just uses different pre-computed arrays from the same fromSpheres() call).
 * 
 * - FROM_LP: Lights loaded from .lp file (only directions are loaded).
 *   - Skip/unskip: No recomputation needed (only affects image-to-light mapping)
 *   - Geometry parameters change: Must call recomputePositions() for SPHERICAL/LIGHTS3D configs
 *     (directions stay fixed, but positions change based on dome geometry)
 *   - Light configuration changes: Must call recomputePositions() when switching to SPHERICAL/LIGHTS3D
 */

class Dome {
public:
	enum LightSource { UNKNOWN, FROM_SPHERES, FROM_LP };

	QString label;
	QString notes;
	std::vector<Eigen::Vector3f> directions;       //estimated infinite far away lights.
	std::vector<Eigen::Vector3f> positionsSphere;  //3d positions, in mm
	std::vector<Eigen::Vector3f> positions3d;      //3d positions, in mm (if imageWidth is defined, otherwise assuming 1).

	//TODO rename
	std::vector<Color3f> ledAdjust;  //multiply pixel valut to correct for led differences
	//TODO dome calibrations as a grid per led

	enum LightConfiguration { DIRECTIONAL, SPHERICAL, LIGHTS3D };
	LightConfiguration lightConfiguration = DIRECTIONAL;
	LightSource lightSource = UNKNOWN;

	double imageWidth = 0.0; //in mm
	double domeDiameter = 0.0;
	double verticalOffset = 0.0;

	Dome();
	Dome(const QString &filename) { load(filename); }
	void fromSpheres(std::vector<Image> &images, std::vector<Sphere *> &spheres, Lens &lens);
	void parseLP(const QString &lp_path);
	void recomputePositions();  //recompute 3d positions from directions when geometry changes
	//TODO: move savelp here from project
	//void saveLP(const QString &lp_path);
	void load(const QString &filename);
	void save(const QString &filename);
	size_t lightsCount() { return directions.size(); }


	QJsonObject toJson();
	void fromJson(const QJsonObject &obj);
};

#endif // DOME_H
