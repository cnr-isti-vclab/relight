#ifndef DEPTHMAP_H
#define DEPTHMAP_H

#include <vector>
#include <QString>
#include <eigen3/Eigen/Core>
/* */
class Camera {
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
	Eigen::Vector3f projectionToImage(Eigen::Vector3f realPosition);
	Eigen::Vector3f applyRadialDistortion(Eigen::Vector3f& u);
	Eigen::Vector3f applyIntrinsicCalibration(Eigen::Vector3f& u);

	Camera() {}
};

class Depthmap {
public:
	uint32_t width, height;
	Eigen::Vector3f resolution;
	Eigen::Vector3f origin;
	std::vector<float> elevation;
	std::vector<Eigen::Vector3f> normals;
	Eigen::Vector3f pixelToRealCoordinates(int pixelX, int pixelY, float pixelZ);

	//Eigen::Vector3f pixelTo3DCoordinates(int pixelX, int pixelY, float depth, const QString &depthXmlPath, const QString &oriXmlPath);

	Depthmap() {}
	bool load(const char *tiff);
	bool loadXml(const QString &xmlPath);
	void computeNormals();
	void saveNormals(const char *filename);
	void saveObj(const char *filename);
	void depthIntegrateNormals();
	void resizeNormals(int factorPowerOfTwo, int step);
//	void getOrientationVector(const QString &xmlPath, Eigen::Matrix3f &rotation, Eigen::Vector3f &center);

};

#endif // DEPTHMAP_H
