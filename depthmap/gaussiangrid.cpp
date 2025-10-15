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
#include <iostream>
#include <queue>

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
	//clamp to border
	float epsilon = 1e-5;
	x = std::max(0.0f, std::min(x, float(width-1) - epsilon));
	y = std::max(0.0f, std::min(y, float(height-1) - epsilon));
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

	{
		vector<float> cloudZ;
		for(auto &v: cloud)
			cloudZ.push_back(v[2]);

		//fitLinear(source, cloudZ, a, b);
		fitLinearRobust(source, cloudZ, a, b, 5, 5.0f);
	}
	vector<Eigen::Vector3f> diff = cloud;
	//TODO: w and h proportional to aspect ratio of camera
	for(size_t i = 0; i < diff.size(); i++) {
		diff[i][2] -= depthmapToCloud(source[i]);
	}

	computeGaussianWeightedGrid(diff);
	imageGrid("beforelaplacian.png");
	fillLaplacian(precision);

// Debug, check if differences are improved
/*	for(size_t i = 0; i < cloud.size(); i++) {
		auto p = cloud[i];
		float t = target(p[0], p[1], source[i]);
		cout << diff[i] << " vs: " << t - p[2] << endl;
	} */
}


void GaussianGrid::fillLaplacian(float precision){
	// Compute mean of known values (fallback only)
	float sum = 0.0f;
	int known = 0;
	for (size_t i = 0; i < values.size(); ++i) {
		if (weights[i] != 0) {
			sum += values[i]; ++known;
		}
	}
	if (known == 0)
		throw QString("No micmac values in gaussian grid.");
	const float mean = sum / known;

	int unknown = 0;
	for (size_t i = 0; i < values.size(); ++i) if (weights[i] == 0) ++unknown;
	if (unknown == 0) return;

	// Front-propagation initialization: assign unknowns when they have assigned neighbors
	const int min_assigned_neighbors = 1; // set to 2 for a smoother start if desired
	std::vector<uint8_t> assigned(values.size(), 0);
	std::queue<int> q;

	for (int y = 0; y < height; ++y) {
		const int row = y * width;
		for (int x = 0; x < width; ++x) {
			const int idx = row + x;
			if (weights[idx] > 0) {
				assigned[idx] = 1;
				q.push(idx);
			}
		}
	}

	auto try_assign = [&](int x, int y) {
		if (x < 0 || x >= width || y < 0 || y >= height) return false;
		const int idx = y * width + x;
		if (assigned[idx]) return false;

		float sum_n = 0.0f;
		int cnt = 0;
		if (y > 0 && assigned[(y - 1) * width + x]) { sum_n += values[(y - 1) * width + x]; ++cnt; }
		if (y < height - 1 && assigned[(y + 1) * width + x]) { sum_n += values[(y + 1) * width + x]; ++cnt; }
		if (x > 0 && assigned[y * width + (x - 1)]) { sum_n += values[y * width + (x - 1)]; ++cnt; }
		if (x < width - 1 && assigned[y * width + (x + 1)]) { sum_n += values[y * width + (x + 1)]; ++cnt; }

		if (cnt >= min_assigned_neighbors) {
			values[idx] = sum_n / cnt;
			assigned[idx] = 1;
			q.push(idx);
			return true;
		}
		return false;
	};

	// BFS wave from known samples
	while (!q.empty()) {
		int idx = q.front(); q.pop();
		int x = idx % width;
		int y = idx / width;

		(void)try_assign(x - 1, y);
		(void)try_assign(x + 1, y);
		(void)try_assign(x, y - 1);
		(void)try_assign(x, y + 1);
	}

	// Fallback for any disconnected leftovers (shouldn't happen on a connected grid)
	for (size_t i = 0; i < values.size(); ++i) {
		if (!assigned[i]) values[i] = mean, assigned[i] = 1;
	}

	// Red-Black Gauss-Seidel with Successive Over-Relaxation (SOR)
	const float omega = 1.9f;    // tune in [1.0, 2.0); ~1.8-1.95 often best
	const int   max_iter = 1000; // fewer iterations thanks to better init

	bool converged = false;
	for (int it = 0; it < max_iter; ++it) {
		float max_delta = 0.0f;

		// Two-color sweep (checkerboard pattern)
		for (int color = 0; color < 2; ++color) {
			for (int y = 0; y < height; ++y) {
				const int row = y * width;
				for (int x = 0; x < width; ++x) {
					if (((x + y) & 1) != color) continue;

					const int idx = row + x;
					if (weights[idx] > 0) continue; // keep known samples fixed

					float sum_neighbors = 0.0f;
					int count_neighbors = 0;

					if (y > 0)           { sum_neighbors += values[(y - 1) * width + x]; ++count_neighbors; }
					if (y < height - 1)  { sum_neighbors += values[(y + 1) * width + x]; ++count_neighbors; }
					if (x > 0)           { sum_neighbors += values[row + (x - 1)];       ++count_neighbors; }
					if (x < width - 1)   { sum_neighbors += values[row + (x + 1)];       ++count_neighbors; }

					if (count_neighbors == 0) continue;

					const float avg = sum_neighbors / count_neighbors;
					const float oldv = values[idx];
					const float newv = oldv + omega * (avg - oldv);
					values[idx] = newv;

					const float d = std::fabs(newv - oldv);
					if (d > max_delta) max_delta = d;
				}
			}
		}

		if (max_delta < precision) {
			qDebug() << "Converged after" << it + 1 << " iterations.";
			converged = true;
			break;
		}
	}

	if (!converged) {
		qDebug() << "Reached maximum iterations without full convergence.";
	}
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
	return h + value(x, y);
}

float GaussianGrid::corrected(float x, float y, float z) {
	float h = depthmapToCloud(z);
	h += value(x, y);
	return cloudToDepthmap(h);
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
	if(min_max.first == values.end()) {
		cerr << "No values in grid!" << endl;
		exit(-1);
	}
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

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			float value = values[x + y * width];
			float normalized_value = (value - z_min) / (z_max - z_min);
			int gray_value = static_cast<int>(normalized_value * 255);
			image.setPixel(x, y, qRgb(gray_value, gray_value, gray_value));
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
