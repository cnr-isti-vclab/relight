#ifndef DEPTHMAP_H
#define DEPTHMAP_H
#include <tiffio.h>
#include <vector>
#include <QtCore/QString>
#include <eigen3/Eigen/Core>
#include "camera.h"
/* */

class Depthmap {
public:
	uint32_t width, height;
	std::vector<float> elevation;
	std::vector<float> mask;
	std::vector<float> weights;

	std::vector<Eigen::Vector3f> normals;


	//Eigen::Vector3f pixelTo3DCoordinates(int pixelX, int pixelY, float depth, const QString &depthXmlPath, const QString &oriXmlPath);

	Depthmap() {}

	bool loadDepth(const char *depth_path);
	bool loadMask(const char *mask_path);
	bool loadNormals(const char *normals_path);
	void saveDepth(const char *depth_path) const;
	void saveMask(const char *depth_path);
	void saveNormals(const char *normals_path);

	float calculateMeanDepth(const std::vector<float>& values);

	void computeNormals();
	void depthIntegrateNormals();
	void resizeNormals(int factorPowerOfTwo, int step = 1);
	float calculateWeight(float x, float y) const; //


protected:
	bool loadTiff(const char *tiff, std::vector<float> &elevation, uint32_t &w, uint32_t &h);
	bool loadTiledTiff(TIFF* inTiff, std::vector<float> &elevation, uint32_t w, uint32_t h,
					   uint32_t tileWidth, uint32_t tileLength, uint32_t bitsPerSample);
	bool loadStripedTiff(TIFF* inTiff, std::vector<float> &elevation, uint32_t& w, uint32_t& h, uint32_t bitsPerSample);
	static void saveTiff(const char *mask_path, const std::vector<float> &values, uint32_t w, uint32_t h, uint32_t bitsPerSample);

};


class CameraDepthmap: public Depthmap {
public:
	Camera camera;

	/*1. load tif filename, convertita in vector3f
	 *
*/

};
#endif // DEPTHMAP_H


