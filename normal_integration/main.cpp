#include <QCoreApplication>
#include <QImage>
#include "../src/bni_normal_integration.h"

#include "../src/getopt.h"
extern int opterr;

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

int main(int argc, char *argv[]) {

	if(argc == 1) {
		help();
		return 1;
	}
	opterr = 0;

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
	//std::vector<float> height_map(w*h, 0);
	//bni_integrate(nullptr, w, h, normals, height_map, k, tolerance, solver_tolerance, max_iterations, max_solver_iterations);
	std::vector<float> height_map = bni_pyramid(nullptr, w, h, normals, k, tolerance, solver_tolerance, max_iterations, max_solver_iterations, scale);
	if(height_map.size() == 0) {
		cerr << "Failed normal integration" << endl;
		return -1;
	}
	if(!ply.isNull()) {
		savePly(ply, w, h, height_map);
	}
}
