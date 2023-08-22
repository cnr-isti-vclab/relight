#ifndef NORMALSIMAGE_H
#define NORMALSIMAGE_H

#include <QString>
#include <QImage>
#include <vector>

class NormalsImage {
public:
	//radial
	bool exponential = false;
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
	void flattenRadial(double binSize = 20);
	void flattenFourier(int padding, double sigma = 20);
};

#endif // NORMALSIMAGE_H
