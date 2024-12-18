#include "flatnormals.h"

#include <QFile>
#include <QTextStream>


#include <complex>
#include <math.h>

#include <Eigen/Dense>
using namespace Eigen;

#include "pocketfft.h"
using namespace pocketfft;

#include <vector>
#include <iostream>
using namespace std;

NormalsImage::~NormalsImage() {

}


void NormalsImage::load(std::vector<double> &_normals, int _w, int _h) {
	normals = _normals;
	w = _w;
	h = _h;
}

void NormalsImage::load(QString filename) {
	img.load(filename);
	w = img.width();
	h = img.height();

	normals.clear();
	normals.reserve(w*h);
	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			QRgb pixel = img.pixel(x, y);
			Vector3d n;
			n[0] = 2*qRed(pixel)/255.0 - 1.0;
			n[1] = 2*qGreen(pixel)/255.0 - 1.0;
			n[2] = 2*qBlue(pixel)/255.0 - 1.0;
			n.normalize();
			normals.push_back(n[0]);
			normals.push_back(n[1]);
			normals.push_back(n[2]);
		}
	}
}

void NormalsImage::save(QString filename) {
	flat = QImage(w, h, QImage::Format_ARGB32);
	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			QRgb pixel = img.pixel(x, y);
			Vector3d n;
			n[0] = normals[3*(x + y*w)];
			n[1] = normals[3*(x + y*w)+1];
			n[2] = normals[3*(x + y*w)+2];

			pixel = qRgb((n[0] + 1.0)*255.0/2.0, (n[1] + 1.0)*255.0/2.0, n[2]*255.0);
			flat.setPixel(x, y, pixel);
		}
	}

	flat.save(filename);
}

void NormalsImage::flattenRadial(double binSize) {
	//don't use normals not flat enough
	double z_threshold = 0.7171;
	vector<double> binCount;
	vector<double> derivatives;
	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			int index = 3*(x + y*w);

			Vector3d n;
			n[0] = normals[index];
			n[1] = normals[index+1];
			n[2] = normals[index+2];

			assert(!isnan(n[0]));
			assert(!isnan(n[1]));
			assert(!isnan(n[2]));

			if(n[2] < z_threshold) continue;

			Vector3d radial;
			radial[0] = x - w/2.0;
			radial[1] = h/2.0 - y;
			radial[2] = 1.0; //TODO Was zero!
			double distance = radial.norm();
			assert(distance > 0);
			radial.normalize();

			double outward = radial.dot(n);
			int bin = (int) floor(distance/binSize);
			if(derivatives.size() < size_t(bin+1)) {
				derivatives.resize(bin+1, 0.0);
				binCount.resize(bin+1, 0);
			}
			derivatives[bin] += outward;
			binCount[bin]++;
		}
	}
	for(size_t i = 0; i < derivatives.size(); i++)
		derivatives[i] /= binCount[i];

#if 0
	QFile csv("bins.csv");
	csv.open(QFile::WriteOnly | QFile::Truncate);
	QTextStream stream(&csv);
	for(int i = 0; i < derivatives.size(); i++)
		stream << i << "," << derivatives[i] << endl;
#endif

	double sum_x = 0;
	double sum_y = 0;
	double sum_xy = 0;
	double sum_x2 = 0;

	for(size_t i = 0; i < derivatives.size(); i++) {
		double x = i * binSize + binSize/2.0;
		sum_x += x;
		sum_y += derivatives[i];
		sum_xy += x * derivatives[i];
		sum_x2 += x*x;
	}

	// means
	double mean_x = sum_x / derivatives.size();
	double mean_y = sum_y / derivatives.size();

	float varx = sum_x2 - sum_x * mean_x;
	float cov = sum_xy - sum_x * mean_y;

	// check for zero varx
	double M = cov / varx;
	double Q = mean_y - M * mean_x;

	//overwrite normals;

	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			Vector3d n;
			n[0] = normals[3*(x + y*w)];
			n[1] = normals[3*(x + y*w) + 1];
			n[2] = normals[3*(x + y*w) + 2];
			Vector3d radial;
			radial[0] = x - w/2.0;
			radial[1] = h/2.0 - y;
			radial[2] = 0;
			double distance = radial.norm();
			double inward = M*distance + Q;
			radial.normalize();
			radial = radial * inward;
			n -= radial;
			n.normalize();
			normals[3*(x + y*w)] = n[0];
			normals[3*(x + y*w) + 1] = n[1];
			normals[3*(x + y*w) + 2] = n[2];

		}
	}
}

void NormalsImage::flattenFourier(int padding, double sigma) {
	padding_amount = padding;
	unsigned int W = w + padding_amount*2;
	unsigned int H = h + padding_amount*2;

	shape_t shape{ W, H };
	stride_t stride { 16, W*16 };
	shape_t axes{0, 1};

	vector<complex<double>> datax(W*H);
	vector<complex<double>> datay(W*H);

	vector<double> dataz(w*h);

	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			Vector3d n;
			n[0] = normals[3*(x + y*w)];
			n[1] = normals[3*(x + y*w)+1];
			n[2] = normals[3*(x + y*w)+2];


			if(exponential) {
				double t = acos(n[2]);
				if(t > 0) {
					n[0] = n[0]*t/sin(t);
					n[1] = n[1]*t/sin(t);
				}
			}

			assert(!isnan(n[0]));
			assert(!isnan(n[1]));
			assert(!isnan(n[2]));


			int X = x + padding_amount;
			int Y = y + padding_amount;
			datax[X + Y*W].real(n[0]);
			datax[X + Y*W].imag(0);
			datay[X + Y*W].real(n[1]);
			datay[X + Y*W].imag(0);
			if(x < padding_amount) {
				X = padding_amount -x;
			}

			if(x >= w - padding_amount) {
				X = W - padding_amount + w -1 - x;
			}

			if(y < padding_amount) {
				Y = padding_amount - y;
			}

			if(y >= h - padding_amount) {
				Y = H - padding_amount + h - 1 -y;
			}

			datax[X + Y*W].real(n[0]);
			datax[X + Y*W].imag(0);
			datay[X + Y*W].real(n[1]);
			datay[X + Y*W].imag(0);

			dataz[x + y*w] = n[2];
		}
	}

	c2c(shape, stride, stride, axes, FORWARD, datax.data(), datax.data(), 1.0);
	c2c(shape, stride, stride, axes, FORWARD, datay.data(), datay.data(), 1.0);


	for(int y = 0; y < H; y++) {
		for(int x = 0; x < W; x++) {
			int X = x;
			if(X > W/2) X -= W;
			int Y = y;
			if(Y > H/2) Y -= H/2;
			double r2 = X*X + Y*Y;
			double g = 1.0 - exp(-0.5*r2/(sigma*sigma));
			/* Using Hann instead?
			 * double g = 1.0;
			if(r2 < sigma*sigma/4)
				g = 1.0 - pow(cos(M_PI*r2/sigma), 2); */
			assert(!isnan(datax[x + y*W].real()));
			assert(!isnan(datay[x + y*W].imag()));

			datax[x + y*W] *= g;
			datay[x + y*W] *= g;
		}
	}

	c2c(shape, stride, stride, axes, BACKWARD, datax.data(), datax.data(), 1./(W*H));
	c2c(shape, stride, stride, axes, BACKWARD, datay.data(), datay.data(), 1./(W*H));


	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			int X  = x + padding_amount;
			int Y = y + padding_amount;
			double r = datax[X + Y*W].real();
			double g = datay[X + Y*W].real();
			double d = sqrt(r*r + g*g);
			if(exponential) {
				if(d > 0) {
					r = r*sin(d)/d;
					g = g*sin(d)/d;
				}
			}

			double b = sqrt(1 - r*r - g*g);

			normals[3*(x + y*w)] = r;
			normals[3*(x + y*w) + 1] = g;
			normals[3*(x + y*w) + 2] = b;

			/*
			 //differences might be nice to export.
			double dr = inx[X + Y*W][0] - r;
			double dg = iny[X + Y*W][0] - g;
			double db = dataz[x + y*w] - b;


			QRgb dpixel = qRgb((dr + 1.0)*255.0/2.0, (dg + 1.0)*255.0/2.0, (db + 1.0)*255.0/2.0);
			residual.setPixel(x, y, dpixel);
			*/
		}
	}
}
