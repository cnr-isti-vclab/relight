#ifndef DEPTHMAP_H
#define DEPTHMAP_H
#include <tiffio.h>
#include <vector>
#include <QString>
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
	void saveTiff(const char *mask_path, const std::vector<float> &values, uint32_t w, uint32_t h, uint32_t bitsPerSample) const;

};


class CameraDepthmap: public Depthmap {
public:
	Camera camera;

	/*1. load tif filename, convertita in vector3f
	 *
*/

};
#endif // DEPTHMAP_H
/*
class OrthoDepthmap:
					   public Depthmap {
public:
	Eigen::Vector3f resolution;
	Eigen::Vector3f origin;
	std::vector<Eigen::Vector3f> point_cloud;
	std::vector<float> x, y, z;
	std::vector<float> z_grid;
	int grid_x, grid_y;
	float sigma;

	Eigen::Vector3f pixelToRealCoordinates(int pixelX, int pixelY, float pixelZ);
	Eigen::Vector3f realToPixelCoord(float realX, float realY, float realZ);
	bool load(const char *depth_path, const char *mask_path);
	bool loadXml(const char *xmlPath);
	void saveObj(const char *filename);
	void projectToCameraDepthMap(const Camera& camera, const QString& outputPath);
	void resizeNormals(int factorPowerOfTwo, int step = 1);
	void loadPointCloud(const char *textPath);
	//itera sui punti, chiama l'inversa, prima converte a intero perche sono float vede se xy stanno in w e h, se non dentro problema
	//legge nella depth l h corrispondente
	void verifyPointCloud();
	void integratedCamera(const CameraDepthmap& camera, const char *outputFile);
	void beginIntegration();
	void endIntegration();


	/*1.


};*/

