#ifndef ORTHODEPTHMAP_H
#define ORTHODEPTHMAP_H
#include <tiffio.h>
#include <vector>
#include <QString>
#include <eigen3/Eigen/Core>
#include "depthmap.h"



class OrthoDepthmap : public Depthmap {
public:

	Eigen::Vector3f resolution;
	Eigen::Vector3f origin;
	std::vector<Eigen::Vector3f> point_cloud;
	//std::vector<float> x, y, z;
	std::vector<float> z_grid;
	std::vector<float> old_elevation;
	std::vector<float> blurred_mask;
	int grid_x, grid_y;
	float sigma;
	float blur = 1.0f;


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
	void saveBlurredMask(const char* filename);


	/*1.
*/

};

#endif // ORTHODEPTHMAP_H
