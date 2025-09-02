#ifndef CAMERA_H
#define CAMERA_H
#include <tiffio.h>
#include <vector>
#include <QString>
#include <eigen3/Eigen/Core>

class Camera
{
public:
	//intrinsic
	uint32_t width = 0, height = 0;
	float focal = 0;
	float PPx = 0, PPy = 0; // principal point
	float Cx = 0, Cy = 0; // center of distorsion
	float R3 = 0, R5 = 0, R7 = 0; // radial distorsion
	//extrinsic
	Eigen::Matrix3f rotation;
	Eigen::Vector3f center;
	bool loadXml(const QString &path); //read the MicMac xml origin, origin resolution ecc.
	bool loadInternParameters(const QString &internePath); // read the xml with the center, rotation, focal parameter, principal points parameters ecc.
	Eigen::Vector3f projectionToImage(Eigen::Vector3f realPosition) const;
	//inverse of the above
	Eigen::Vector3f projectionToReal(Eigen::Vector3f imgPosition) const;

	Eigen::Vector3f applyRadialDistortion(Eigen::Vector3f& u);
	Eigen::Vector3f applyIntrinsicCalibration(Eigen::Vector3f& u) const;
};

#endif // CAMERA_H
