#ifndef GAUSSIANGRID_H
#define GAUSSIANGRID_H
#include <tiffio.h>
#include <vector>
#include <QString>
#include <eigen3/Eigen/Core>

class GaussianGrid
{

public:
	int width, height;
	std::vector<float> values;
	std::vector<float> weights;
	float sideFactor = 0.5f;  // corrective factor over the 1/sqrt(n) formula.
	int minSamples = 3;       // minimum number of samples needed in a pixel
	float sigma;
	float a, b;//coefficient of linear transform from source to point cloud space.

	void init(std::vector<Eigen::Vector3f> &cloud, std::vector<float> &source);
	void fitLinear(std::vector<float> &x, std::vector<float> &y, float &a, float &b); //ax + b
	void fitLinearRobust(std::vector<float> &x, std::vector<float> &y,
									   float &a, float &b,
									   int iterations, float thresh);
	float bilinearInterpolation(float x, float y);
	void computeGaussianWeightedGrid(std::vector<Eigen::Vector3f> &differences);
	void fillLaplacian(int w, int h, std::vector<float> &values, std::vector<float> &weights, float precision);
	void imageGrid(const char* filename);
	//interpola la griglia per spostare la depthmap, serve per creare la funzione
	float value(float x, float y);

	//given a point in the source depthmap compute the z in cloud coordinate space;
	float target(float x, float y, float source);

	//corrected elevation in source coordinates
	float corrected(float x, float y, float source);



	float depthmapToCloud(float z) {
		return a*z + b;
	}
	float cloudToDepthmap(float h) {
		return (h - b)/a;
	}


};

#endif // GAUSSIANGRID_H
