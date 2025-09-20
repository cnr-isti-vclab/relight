#ifndef NORMALSIMAGE_H
#define NORMALSIMAGE_H

#include <QString>
#include <QImage>
#include <vector>


void flattenBlurNormals(int w, int h, std::vector<float> &normals, double sigma = 10.0);
void flattenRadialNormals(int w, int h, std::vector<float> &normals, double binSize = 20.0);
void flattenFourierNormals(int w, int h, std::vector<float> &normals, float padding = 0.2, double sigma = 20, bool exponential = true);

void flattenRadialHeights(int w, int h, std::vector<float> &heights, double binSize = 20.0);
void flattenFourierHeights(int w, int h, std::vector<float> &heights, float padding = 0.2, double sigma = 20);

class NormalsImage {
public:
	//radial
	bool exponential = true;
	//fourier
	double sigma = 20;
	int padding_amount = 20;

	int w, h;
	QImage img, flat;
	std::vector<double> normals;


	~NormalsImage();

	void load(std::vector<double> &normals, int w, int h);
	void load(QString filename);
	void save(QString filename);

	//fit a line radial normal divergence, bin size 20px

};

#endif // NORMALSIMAGE_H
