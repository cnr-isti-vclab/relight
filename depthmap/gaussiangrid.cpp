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

	fitLinear(source, cloudZ, a, b);
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


vector<float> GaussianGrid::blurMask(const vector<float>& mask, int w, int h, float sigma) {
	Grid<float> img(w, h);
	for (size_t i = 0; i < mask.size(); i++) img[i] = mask[i];
	Grid<float> blurred = img.gaussianBlur((int)(round(sigma) * 6 + 1), sigma);
	vector<float> out(mask.size());
	for (size_t i = 0; i < out.size(); ++i) out[i] = blurred[i];
	return out;
}
//carica la mask, applica il gaussian blur con il peso scalato da 0.5 a 1 fai la somma pesata tra la depth di Mic Mac e quella ricavata dal gaussian blur
//gaussian, maschera 0.5 1, peso mediato

vector<float> GaussianGrid::loadAndBlurMaskWeights(QString mask_path, int expected_w, int expected_h, float sigma) {
	QImage mask(mask_path);
	if (mask.width() != expected_w || mask.height() != expected_h) {
		throw std::runtime_error("Mask size mismatch");
	}

	vector<float> raw(expected_w * expected_h);
	for (int y = 0; y < expected_h; y++) {
		for (int x = 0; x < expected_w; x++) {
			int i = x + y * expected_w;
			float val = qGray(mask.pixel(x, y)) / 255.0f;
			raw[i] = val;
		}
	}

	auto blurred = blurMask(raw, expected_w, expected_h, sigma);

	// Scala da [0, 1] → [0.5, 1.0]
	for (size_t i = 0; i < blurred.size(); i++) {
		blurred[i] = 0.5f + 0.5f * blurred[i];
	}

	return blurred;
}





//scala tra 0 e 1. img

