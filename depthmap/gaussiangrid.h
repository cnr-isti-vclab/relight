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
	float bilinearInterpolation(float x, float y);
	void computeGaussianWeightedGrid(std::vector<Eigen::Vector3f> &differences);
	void fillLaplacian(float precision);
	void imageGrid(const char* filename);
	//interpola la griglia per spostare la depthmap, serve per creare la funzione
	float value(float x, float y);
	float target(float x, float y, float source); //given a point in the source depthmap compute the z in cloud coordinate space;



	float depthmapToCloud(float h) {
		return a*h + b;
	}


};

#endif // GAUSSIANGRID_H
