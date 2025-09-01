#include "gaussiangrid.h"
#include <tiffio.h>
#include <iostream>
#include <QImage>
#include <QDir>
#include <QDomElement>
#include <QtXml/QDomDocument>
#include "depthmap.h"
#include "../src/bni_normal_integration.h"
#include <QFile>
#include <QDebug>
#include <QRegularExpression>
#include <fstream>

using namespace std;


void GaussianGrid::fitLinear(std::vector<float> &x, std::vector<float> &y, float &a, float &b) {
	if (x.size() != y.size()) {
		cout << "Errore: i vettori x e y devono avere la stessa lunghezza." << endl;
		return;
	}

	int n = x.size();
	float sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_x2 = 0.0;

	for (int i = 0; i < n; i++) {
		sum_x += x[i];
		sum_y += y[i];
		sum_xy += x[i] * y[i];
		sum_x2 += x[i] * x[i];
	}

	// Calcolo dei coefficienti a e b
	a = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
	b = (sum_y - a * sum_x) / n;

	std::ofstream out("xy_fit_data.csv");
	if (out.is_open()) {
		out << "x\ty\n";
		for (int i = 0; i < n; i++) {
			out << x[i] << "\t" << y[i] << "\n";
		}
	}
}

void GaussianGrid::fitLinearRobust(std::vector<float> &x, std::vector<float> &y,
								   float &a, float &b,
								   int iterations, float thresh) {
	if (x.size() != y.size()) {
		cerr << "Errore: i vettori x e y devono avere la stessa lunghezza." << endl;
		return;
	}

	int n = x.size();
	std::vector<bool> mask(n, true);

	for (int it = 0; it < iterations; it++) {
		std::vector<float> x_used, y_used;
		for (int i = 0; i < n; i++) {
			if (mask[i]) {
				x_used.push_back(x[i]);
				y_used.push_back(y[i]);
			}
		}

		if (x_used.size() < 2) break;

		// fit linear on point active
		fitLinear(x_used, y_used, a, b);

		// residui
		std::vector<float> errs;
		for (int i = 0; i < n; i++) {
			if (!mask[i]) continue;
			float e = std::fabs(y[i] - (a * x[i] + b));
			errs.push_back(e);
		}
		if (errs.empty()) break;

		std::sort(errs.begin(), errs.end());
		float med = errs[errs.size() / 2];

		// reject outliers
		int rejected = 0;
		for (int i = 0; i < n; i++) {
			float e = std::fabs(y[i] - (a * x[i] + b));
			bool keep = (e <= thresh * med);
			if (mask[i] && !keep) rejected++;
			mask[i] = keep;
		}

		cout << "[RobustFit] Iter " << it+1
				  << " a=" << a << " b=" << b
				  << " med=" << med
				  << " rejected=" << rejected
				  << " kept=" << std::count(mask.begin(), mask.end(), true)
				  << "/" << n
				  << endl;

		if (rejected == 0) break; // convergenza
	}

	//
	std::ofstream out("xy_fit_robust.csv");
	if (out.is_open()) {
		out << "x,y,fit,used\n";
		for (int i = 0; i < n; i++) {
			float y_fit = a * x[i] + b;
			out << x[i] << "," << y[i] << "," << y_fit << "," << (mask[i] ? 1 : 0) << "\n";
		}
	}
}





float GaussianGrid::bilinearInterpolation(float x, float y) {

	float x1 = floor(x);
	float y1 = floor(y);
	float x2 = x1+1;
	float y2 = y1+1;


	if (x1 < 0 || x2 >= width || y1 < 0 || y2 >= height) {
		cerr << "Coordinate fuori dai limiti della griglia!" << endl;
		return 0.0f;
	}


	float Q11 = values[x1 + y1 * width];
	float Q12 = values[x1 + y2 * width];
	float Q21 = values[x2 + y1 * width];
	float Q22 = values[x2 + y2 * width];

	float R1 = (x2 - x) * Q11 + (x - x1) * Q21;
	float R2 = (x2 - x) * Q12 + (x - x1) * Q22;

	float P = (y2 - y) * R1 + (y - y1) * R2;

	return P;
}
//fit h = a+b*elev
void GaussianGrid::init(std::vector<Eigen::Vector3f> &cloud, std::vector<float> &source) {
	int side = static_cast<int>(sideFactor * sqrt(cloud.size()));
	sigma = 1.0f / side;
	width = side;
	height = side;
	float precision = 0.00001f;

	vector<float> cloudZ;
	for(auto &v: cloud)
		cloudZ.push_back(v[2]);

	//fitLinear(source, cloudZ, a, b);
	fitLinearRobust(source, cloudZ, a, b, 5, 5.0f);
	//exit(0);
	//TODO: w and h proportional to aspect ratio of camera
	for(size_t i = 0; i < cloud.size(); i++) {
		cloud[i][2] -= depthmapToCloud(source[i]);
	}

	computeGaussianWeightedGrid(cloud);
	fillLaplacian(precision);

}


void GaussianGrid::fillLaplacian(float precision){
	float mean = 0;
	float count = 0;
	for(int i = 0; i < values.size(); i++){
		if(weights[i] != 0){
			mean += values[i];
			count++;
		}
	}
	mean/=count;
	for(int i = 0; i < values.size(); i++){
		if(weights[i] == 0){
			values[i] = mean;
		}
	}

	std::vector<float> old_values = values;
	std::vector<float> new_values = values;

	bool converged = false;
	float max_iter = 1000;

	for (int i = 0; i < max_iter; i++) {

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; ++x) {
				int index = y * width + x;

				if (weights[index] > 0) {
					continue;
				}

				float sum_neighbors = 0.0f;
				int count_neighbors = 0;

				//neighbors up
				if (y > 0) {
					sum_neighbors += old_values[(y - 1) * width + x];
					count_neighbors++;
				}

				// neigh down
				if (y < height - 1) {
					sum_neighbors += old_values[(y + 1) * width + x];
					count_neighbors++;
				}

				// neigh left
				if (x > 0) {
					sum_neighbors += old_values[y * width + (x - 1)];
					count_neighbors++;
				}

				// neigh right
				if (x < width - 1) {
					sum_neighbors += old_values[y * width + (x + 1)];
					count_neighbors++;
				}

				// update the value
				if (count_neighbors > 0) {
					new_values[index] = sum_neighbors / count_neighbors;
				}
			}
		}

		float max_difference = 0.0f;
		for (size_t i = 0; i < values.size(); ++i) {
			max_difference = std::max(max_difference, std::abs(new_values[i] - old_values[i]));
		}

		if (max_difference < precision) {
			qDebug() << "Converged after" << i + 1 << "iterations.";
			converged = true;
			break;
		}

		old_values = new_values;
	}

	if (!converged) {
		qDebug() << "Reached maximum iterations without full convergence.";
	}

	values = new_values;
}

float GaussianGrid::value(float x, float y){
	//bicubic interpolation
	//nearest
	float pixelX = x * (width-1);
	float pixelY = y * (height-1);

	return bilinearInterpolation(pixelX, pixelY);

	//return values[pixelX + pixelY * width];

}

float GaussianGrid::target(float x, float y, float h) {
	h = depthmapToCloud(h);
	return h;
	return h + value(x, y);
}

void GaussianGrid::computeGaussianWeightedGrid(std::vector<Eigen::Vector3f> &differences) {

	float x_min = 0;
	float x_max = 1;
	float y_min = 0;
	float y_max = 1;

	float x_step = (x_max - x_min) / (width - 1);
	float y_step = (y_max - y_min) / (height - 1);

	values.resize(width * height, 0);
	weights.resize(width * height, 0);

	std::vector<int> count(width*height, 0);


	float max_distance = 2 * sigma;
	for (auto &p : differences) {


		int x_start = std::max(0, static_cast<int>((p[0] - max_distance - x_min) / x_step));
		int x_end = std::min(width - 1, static_cast<int>((p[0] + max_distance - x_min) / x_step));
		int y_start = std::max(0, static_cast<int>((p[1] - max_distance - y_min) / y_step));
		int y_end = std::min(height - 1, static_cast<int>((p[1] + max_distance - y_min) / y_step));

		for (int x = x_start; x <= x_end; x++) {
			for (int y = y_start; y <= y_end; y++) {
				float xg = x_min + x * x_step;
				float yg = y_min + y * y_step;

				float distance = sqrt((p[0] - xg) * (p[0] - xg) + (p[1] - yg) * (p[1] - yg));
				if (distance <= max_distance) {
					float weight = exp(-(distance * distance) / (2 * sigma * sigma));
					values[y * width + x] += weight * p[2];
					weights[y * width + x] += weight;
					count[y*width + x]++;
				}
			}
		}
	}
	//chiama camere tutte e 4 e vedi come vengono
	// pesare per il blanding funzione intervallo 0, 1 * 0, 1 0 ai bordi 1 al centro, che sia una funzione continua
	//polinomio di 2 grado in x * pol 2 grado in y. derivata e peso a 0 sul bordo
	//fai somma pesata e veedi come vieni
	//funz target ritorna valore e peso

	for (int i = 0; i < values.size(); i++) {
		if(count[i] < minSamples)
			weights[i] = 0;
		if (weights[i] != 0) {
			values[i] /= (weights[i]);
		}
	}
	/*for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			qDebug() << "z_grid[" << i << "][" << j << "] = " << values[i * height +j];
		}
	}*/
}
//se < 1/4 è -8x^2, else se è < 3/4. 8*(x-1)^2, data una x mi ritorna una x+1



void GaussianGrid::imageGrid(const char* filename) {

	auto min_max = minmax_element(values.begin(), values.end());
	float z_min = *min_max.first;
	float z_max = *min_max.second;

	if (z_max == z_min) {
		cerr << "All values in z_grid are the same. Cannot normalize." << endl;
		return;
	}

	QImage image(width, height, QImage::Format_Grayscale8);
	if (!QFile::exists(QFileInfo(filename).absolutePath())) {
		qDebug() << "Directory does not exist: " << QFileInfo(filename).absolutePath();
		return;
	}

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			float value = values[i * width + j];
			float normalized_value = (value - z_min) / (z_max - z_min);
			int gray_value = static_cast<int>(normalized_value * 255);
			image.setPixel(j, i, qRgb(gray_value, gray_value, gray_value));
		}
	}
	image.save(filename, "png");
}




//scala tra 0 e 1. img






//robust fit
/* #include <stdio.h>
#include <math.h>
#include <stdbool.h>

typedef struct {
	double a; // slope
	double b; // intercept
} LinReg;

LinReg fit_linear(const double *x, const double *y, const bool *mask, int n) {
	double sumx=0, sumy=0, sumxy=0, sumxx=0;
	int count=0;
	for (int i=0; i<n; i++) {
		if (!mask[i]) continue;
		sumx  += x[i];
		sumy  += y[i];
		sumxy += x[i]*y[i];
		sumxx += x[i]*x[i];
		count++;
	}

	LinReg r = {0,0};
	double denom = count*sumxx - sumx*sumx;
	if (denom != 0 && count>1) {
		r.a = (count*sumxy - sumx*sumy) / denom;
		r.b = (sumy - r.a*sumx) / count;
	}
	return r;
}

LinReg robust_fit(const double *x, const double *y, int n, int iterations, double thresh) {
	bool mask[n];
	for (int i=0; i<n; i++) mask[i] = true;

	LinReg r = {0,0};
	for (int it=0; it<iterations; it++) {
		r = fit_linear(x,y,mask,n);

		// compute residuals
		double errs[n];
		int count=0;
		for (int i=0; i<n; i++) {
			if (!mask[i]) continue;
			errs[count++] = fabs(y[i] - (r.a*x[i] + r.b));
		}

		// compute median error
		if (count == 0) break;
		for (int i=0;i<count-1;i++) // tiny selection sort for median
			for (int j=i+1;j<count;j++)
				if (errs[j] < errs[i]) {
					double tmp=errs[i]; errs[i]=errs[j]; errs[j]=tmp;
				}
		double med = errs[count/2];

		// reject outliers
		for (int i=0; i<n; i++) {
			double e = fabs(y[i] - (r.a*x[i] + r.b));
			mask[i] = (e <= thresh*med);
		}
	}
	return r;
}

int main(void) {
	double x[] = {1,2,3,4,5,6,7};
	double y[] = {2,4,6,8,100,12,14}; // note the outlier at y=100
	int n = sizeof(x)/sizeof(x[0]);

	LinReg r = robust_fit(x,y,n,5,3.0); // 5 iterations, reject >3×median error
	printf("Robust fit: y = %.3f*x + %.3f\n", r.a, r.b);
	return 0;
}
*/
