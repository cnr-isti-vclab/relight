#include "flatnormals.h"
#include <assm/Grid.h>

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

void flattenRadialNormals(int w, int h, std::vector<Eigen::Vector3f> &normals, double binSize) {

	//don't use normals not flat enough
	double z_threshold = 0.7171;
	vector<double> binCount;
	vector<double> derivatives;
	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			int index = 3*(x + y*w);

			Vector3d n;
			n[0] = double(normals[index/3][0]);
			n[1] = double(normals[index/3][1]);
			n[2] = double(normals[index/3][2]);

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
			int idx = 3*(x + y*w);
			n[0] = double(normals[idx/3][0]);
			n[1] = double(normals[idx/3][1]);
			n[2] = double(normals[idx/3][2]);
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
			int pos = x + y*w;
			normals[pos][0] = float(n[0]);
			normals[pos][1] = float(n[1]);
			normals[pos][2] = float(n[2]);

		}
	}
}


void flattenRadialHeights(int w, int h, std::vector<float> &heights, double binSize) {

	vector<double> z(heights.size());

	//subsampling?
	Eigen::MatrixXd A(w*h, 6);
	for(int y = 0; y < h; y++) {
		for(int x= 0; x < w; x++) {
			int i = x + y*w;
			A(i, 0) = 1;
			A(i, 1) = x;
			A(i, 2) = y;
			A(i, 3) = x * x;
			A(i, 4) = y * y;
			A(i, 5) = x * y;
			z[i] = double(heights[i]);
		}
	}

	Eigen::VectorXd b = Eigen::Map<const Eigen::VectorXd>(z.data(), w*h);

	// Solve  A^T * A * coeffs = A^T * b
	Eigen::VectorXd coeffs = (A.transpose() * A).ldlt().solve(A.transpose() * b);
	cout << coeffs << endl;

	for(int y = 0; y < h; y++) {
		for(int x= 0; x < w; x++) {
			int i = x + y*w;
			double h = coeffs(0) + coeffs(1) * x + coeffs(2) * y
					+ coeffs(3) * x * x + coeffs(4) * y * y
					+ coeffs(5) * x * y;
			heights[i]  = float(z[i] - h);
		}
	}
}


void flattenBlurNormals(int w, int h, std::vector<Eigen::Vector3f> &normals, double sigma) {
	Grid<Eigen::Vector2f> img(w, h, Eigen::Vector2f(0, 0));
	for(size_t i = 0; i < normals.size(); i++) {
		img[i] = Eigen::Vector2f(normals[i][0], normals[i][1]);
	}
	//resize img properly.
	Grid<Eigen::Vector2f> blurred;

	int nw = round(2.0f * w / sigma);
	int nh = round(2.0f * h / sigma);
	if(nw >= w || nh >= h) {
		blurred = img.gaussianBlur((int)(round(sigma)*6+1), sigma);
	} else {
		Grid<Eigen::Vector2f> small = img.downscale(nw, nh);
		sigma = 2.0f;
		Grid<Eigen::Vector2f> small_blurred = small.gaussianBlur((int)(round(sigma)*6+1), sigma);
		blurred = small_blurred.upscale(w, h);
	}
	for(size_t i = 0; i < blurred.size(); i++) {
		Eigen::Vector2f v = img[i] - blurred[i];
		normals[i][0] = v[0];
		normals[i][1] = v[1];
		normals[i][2] = sqrt(1 - v[0]*v[0] + v[1]*v[1]);
	}
}

void flattenFourierNormals(int w, int h, std::vector<Eigen::Vector3f> &normals, float padding, double sigma, bool exponential) {

	int padding_amount = round(padding* std::min(w, h));
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
			int idx = 3*(x + y*w);
			n[0] = double(normals[idx/3][0]);
			n[1] = double(normals[idx/3][1]);
			n[2] = double(normals[idx/3][2]);


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

			int pos = x + y*w;
			normals[pos][0] = float(r);
			normals[pos][1] = float(g);
			normals[pos][2] = float(b);

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

void filterLowFrequencies(int w, int h, std::vector<std::complex<double>>& freq_data, double cutoff) {
	for (size_t y = 0; y < h; ++y) {
		for (size_t x = 0; x < w; ++x) {
			double fx = (x <= w / 2) ? x : x - w;
			double fy = (y <= h / 2) ? y : y - h;
			double freq_mag = std::sqrt(fx * fx + fy * fy);

			if (freq_mag < cutoff) {
				freq_data[y * w +x] = 0.0;
			}
		}
	}
}

//TODO do the padding!
void flattenFourierHeights(int w, int h, std::vector<float> &heights, float padding, double sigma) {

	vector<double> heightmap(w*h);
	for(size_t i = 0; i < heights.size(); i++)
		heightmap[i] = heights[i];

	pocketfft::shape_t shape = { size_t(w), size_t(h) };
	pocketfft::stride_t stride = { ptrdiff_t(sizeof(double)), ptrdiff_t(sizeof(double)*w) };
	pocketfft::stride_t stride_freq = { ptrdiff_t(sizeof(std::complex<double>)), ptrdiff_t(sizeof(std::complex<double>)*w) };
	pocketfft::shape_t axes{0, 1};

	size_t n_elements = w*h;

	std::vector<std::complex<double>> freq_data(n_elements);

	pocketfft::r2c<double>(shape, stride, stride_freq, axes, FORWARD, heightmap.data(), freq_data.data(), 1.0);

	filterLowFrequencies(w, h, freq_data, sigma);

	pocketfft::c2r<double>(shape, stride_freq, stride, axes, BACKWARD, freq_data.data(), heightmap.data(), 1.0 / n_elements);

	for(size_t i = 0; i < heights.size(); i++)
		heights[i] = heightmap[i];
}
