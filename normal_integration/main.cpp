#include <QCoreApplication>
#include <QImage>
#include <QFile>

#include "../src/bni_normal_integration.h"

#include "../src/getopt.h"
extern int opterr;

#include <math.h>
#include <iostream>
using namespace std;

void help() {
	cout << "Integrates a normal map into a surface\n\n";
	cout << "Usage: normal_integration [-pkmMt][INPUT]\n\n";
	cout << " -p <filename.ply>: export surface in ply format\n";
	cout << " -k <float>: how likely is a discontinuity to be considered[0-10], default 2\n";
	cout << " -m <integer>: max number of iterations in each step (default: 1000)\n";
	cout << " -M <integer>: max number of discontinuity iterations (default 150)\n";
	cout << " -t <float>: solver target error (default 1e-4)\n";
	cout << " -T <float>: minimum energy discontinuity improvement (default 1e-4)\n";
	cout << " -s <int>: halve images size n times.\n";
}

void ensure(bool condition, const std::string &msg) {
	if(condition)
		return;
	cerr << msg << endl;
	exit(-1);
}

void saveBlob(QString filename, int w, int h, vector<float> &height) {
	QFile file(filename);
	if(!file.open(QFile::WriteOnly)) {
		cerr << "Failed saving: " << qPrintable(filename);
		return;
	}
	file.write((const char *)&w, 4);
	file.write((const char *)&h, 4);
	file.write((const char *)&height[0], 4*height.size());
}

void readBlob(QString filename, int &w, int &h, vector<float> &height) {
	QFile file(filename);
	if(!file.open(QFile::ReadOnly)) {
		cerr << "Could not open file: " << qPrintable(filename);
		return;
	}
	file.read((char *)&w, 4);
	file.read((char *)&h, 4);
	height.resize(w* h);
	file.read((char *)&height[0], w*h*4);
}

void averageFilter(int window, int w, int h, vector<float> &height, vector<float> &filtered) {

	filtered.resize(w*h);

	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			int pos = x + y*w;
			double weight = 0;
			double average = 0.0;

			for(int dy = -window; dy <= window; dy++) {
				if(y + dy < 0 || y + dy >= h)
					continue;
				for(int dx = -window; dx <= window; dx++) {
					if(x + dx < 0 || x + dx >= w)
						continue;

					int dpos = x + dx + (y + dy)*w;
					double sigma = window/2.0;
					double gaussian = exp(-0.5*(dx*dx + dy*dy)/sigma*sigma)/(sigma*2*sqrt(M_PI));
					average += gaussian * height[dpos];
					weight += gaussian;
				}
			}
			double diff = height[pos] - average/weight;

			filtered[pos] = diff;
		}
	}
}

// Function to find both minimum and maximum percentiles using a histogram
std::pair<float, float> findMinMaxPercentilesWithHistogram(
	const std::vector<float>& data, float minPercentile, float maxPercentile, size_t numBins = 1000)
{
	if (data.empty()) {
		throw std::invalid_argument("The input vector is empty.");
	}
	if (minPercentile < 0.0 || minPercentile > 100.0 ||
		maxPercentile < 0.0 || maxPercentile > 100.0 || minPercentile > maxPercentile) {
		throw std::invalid_argument("Percentiles must be between 0 and 100, and minPercentile <= maxPercentile.");
	}

	// Find the range of the data
	auto minMax = std::minmax_element(data.begin(), data.end());
	float minValue = *minMax.first;
	float maxValue = *minMax.second;

	if (minValue == maxValue) {
		return {minValue, maxValue}; // All values are the same
	}

	// Define bin width
	float binWidth = (maxValue - minValue) / numBins;

	// Create histogram
	std::vector<size_t> histogram(numBins, 0);
	for (float value : data) {
		size_t binIndex = std::min(static_cast<size_t>((value - minValue) / binWidth), numBins - 1);
		histogram[binIndex]++;
	}

	// Calculate cumulative frequencies
	size_t totalElements = data.size();
	size_t minTargetCount = static_cast<size_t>(std::ceil((minPercentile / 100.0) * totalElements));
	size_t maxTargetCount = static_cast<size_t>(std::ceil((maxPercentile / 100.0) * totalElements));

	size_t cumulativeCount = 0;
	float minPercentileValue = minValue;
	float maxPercentileValue = maxValue;
	bool foundMin = false, foundMax = false;

	for (size_t i = 0; i < numBins; ++i) {
		cumulativeCount += histogram[i];

		// Find the minPercentile value
		if (!foundMin && cumulativeCount >= minTargetCount) {
			float lowerBound = minValue + i * binWidth;
			float upperBound = lowerBound + binWidth;
			float binFraction = (minTargetCount - (cumulativeCount - histogram[i])) / static_cast<float>(histogram[i]);
			minPercentileValue = lowerBound + binFraction * (upperBound - lowerBound);
			foundMin = true;
		}

		// Find the maxPercentile value
		if (!foundMax && cumulativeCount >= maxTargetCount) {
			float lowerBound = minValue + i * binWidth;
			float upperBound = lowerBound + binWidth;
			float binFraction = (maxTargetCount - (cumulativeCount - histogram[i])) / static_cast<float>(histogram[i]);
			maxPercentileValue = lowerBound + binFraction * (upperBound - lowerBound);
			foundMax = true;
		}

		if (foundMin && foundMax) {
			break; // Stop early if both percentiles are found
		}
	}

	return {minPercentileValue, maxPercentileValue};
}
int main(int argc, char *argv[]) {


	if(argc == 1) {
		help();
		return 1;
	}
	opterr = 0;

	std::vector<float> height;
	int W, H;
	readBlob("heightmap.ply.blob", W, H, height);
	std::vector<float> filtered;
	averageFilter(10, W, H, height, filtered);
	//colorize:

	auto boundaries = findMinMaxPercentilesWithHistogram(filtered, 0.05, 100);
	float min = boundaries.first;
	float max = boundaries.second;

	auto minMax = std::minmax_element(filtered.begin(), filtered.end());
	float minValue = *minMax.first;
	float maxValue = *minMax.second;
	cout << "Real min: " << minValue << " percent min: " << min << endl;
	cout << "Real max: " << maxValue << " percent max: " << max << endl;
	QImage img(W, H, QImage::Format_ARGB32);
	for(int y = 0; y < H; y++) {
		for(int x = 0; x < W; x++) {
			float v = 2*(filtered[x + y*W] - min)/(max - min);
			if(v > 1)	 v = 1;
			int l = (int)(v*255.0f);
			img.setPixel(x, y, qRgb(l, l, l));
		}
	}
	img.save("heightmap.jpg");
	return 0;

	QString ply;
	float k = 2; //discontinuity propensity
	int max_solver_iterations = 5000;
	int max_iterations = 150;
	float solver_tolerance = 1e-5;
	float tolerance = 1e-5;
	char c;
	int scale = 0;
	while ((c  = getopt (argc, argv, "hp:k:m:M:t:T:s:")) != -1) {
		switch (c) {
		case 'h': help();
			break;
		case 'p': ply = optarg;
			break;
		case 'k': k = atof(optarg);
			break;
		case 'm': max_solver_iterations = atoi(optarg);
			break;
		case 'M': max_iterations = atoi(optarg);
			break;
		case 't': solver_tolerance = atof(optarg);
			break;
		case 'T': tolerance = atof(optarg);
			break;
		case 's': scale = atoi(optarg);
			break;
		}
	}

	ensure(k >= 0, "K parameter needs to be >= 0");
	ensure(max_solver_iterations > 0, "m parameter (max solver iterations) needs to be positive");
	ensure(max_iterations > 0, "M parameter (max discontinuity iterations) needs to be positive");
	ensure(solver_tolerance > 0, "t parameter (solver convergence tolerance) needs to be > 0");
	ensure(tolerance > 0, "T parameter (discontinuity convergence tolerance) needs to be > 0");
	ensure(optind < argc, "No normal map specified");

	QString filename = argv[optind++];
	QImage normal_map(filename);
	if(normal_map.isNull()) {
		cerr << "Failed to load image: " << qPrintable(filename) << endl;
		return -1;
	}

	int w = normal_map.width();
	int h = normal_map.height();

	std::vector<float> normals;
	for(int y = 0; y < h; y++) {
		QRgb *line = reinterpret_cast<QRgb*>(normal_map.scanLine(y));
		for(int x = 0; x < w; x++) {
			normals.push_back((2.0*qRed(line[x]))/255.0-1.0);
			normals.push_back((2.0*qGreen(line[x]))/255.0 -1.0);
			normals.push_back(-((2.0*qBlue(line[x]))/255.0-1.0));
		}
	}
	std::vector<float> height_map(w*h, 0);
	bni_integrate(nullptr, w, h, normals, height_map, k, tolerance, solver_tolerance, max_iterations, max_solver_iterations);
	//std::vector<float> height_map = bni_pyramid(nullptr, w, h, normals, k, tolerance, solver_tolerance, max_iterations, max_solver_iterations, scale);
	if(height_map.size() == 0) {
		cerr << "Failed normal integration" << endl;
		return -1;
	}

	if(!ply.isNull()) {
		savePly(ply, w, h, height_map);
		saveBlob(ply + ".blob", w, h, height_map);
	}
}
