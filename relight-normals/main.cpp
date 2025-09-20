#include <QCoreApplication>
#include <QImage>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegExp>
#include <QStringList>

#include "../src/normals/bni_normal_integration.h"
#include "../src/normals/fft_normal_integration.h"
#include "../src/normals/flatnormals.h"
#include "../src/normals/normals_parameters.h"
#include "../src/project.h"
#include "../src/imageset.h"
#include "../external/assm/algorithms/Integration.h"

#include "../src/getopt.h"
extern int opterr;

#include <math.h>
#include <iostream>
#include <algorithm>
#include <getopt.h>
using namespace std;

struct ExtendedNormalsParameters : public NormalsParameters {
	// Additional parameters specific to the command line tool
	QString input_folder;
	QString input_project;
	QString input_normalmap;
	QString lp_file;
	
	// Normal generation
	bool generate_normals = false;
	
	// Light geometry (for folder input)
	bool use_3d_lights = false;
	double dome_radius = 0.0;
	double dome_offset = 0.0;
	
	// Crop options
	bool use_crop = false;
	int crop_width = 0;
	int crop_height = 0;
	int crop_x = 0;
	int crop_y = 0;
	
	// Additional flattening parameters
	double blur_sigma = 10.0;
	double fourier_sigma = 20.0;
	double fourier_padding = 0.2;
	bool fourier_exponential = true;
	
	// BNI additional parameters
	double bni_tolerance = 1e-5;
	double bni_solver_tolerance = 1e-5;
	int bni_max_iterations = 10;
	int bni_max_solver_iterations = 500;
	
	// Output options
	QString output_ply;
	QString output_normalmap;
	QString output_heightmap;
	int scale_down = 0;
};

void help() {
	cout << "Relight Normals - Generate normal maps and integrate surfaces\n\n";
	cout << "Usage: relight-normals [options] <input>\n\n";
	
	cout << "Input options (choose one):\n";
	cout << "  <folder>              : Process folder with .lp file\n";
	cout << "  -r <project.relight>  : Process .relight project file\n";
	cout << "  -n <normalmap.png>    : Process existing normal map\n\n";
	
	cout << "Normal generation options (for folder/project input):\n";
	cout << "  -s <solver>           : Normal solver: l2, sbl, rpca (default: l2)\n";
	cout << "  -3 <radius[:offset]>  : 3D light positions, dome radius and optional offset\n";
	cout << "  -k <w>x<h>+<x>+<y>    : Crop to width x height at offset x,y\n\n";
	
	cout << "Flattening options:\n";
	cout << "  --flat-radial <pct>   : Radial flattening with percentage (default: 20)\n";
	cout << "  --flat-fourier <sigma[:padding[:exp]]> : Fourier flattening\n";
	cout << "  --flat-blur <sigma>   : Blur flattening with sigma (default: 10)\n\n";
	
	cout << "Integration options:\n";
	cout << "  -i <method>           : Integration method: bni, fft, assm (default: none)\n";
	cout << "  --bni-k <float>       : BNI discontinuity parameter (default: 2.0)\n";
	cout << "  --bni-tol <float>     : BNI tolerance (default: 1e-5)\n";
	cout << "  --bni-iter <int>      : BNI max iterations (default: 10)\n";
	cout << "  --assm-error <float>  : ASSM target error (default: 0.1)\n\n";
	
	cout << "Output options:\n";
	cout << "  -p <file.ply>         : Export surface as PLY\n";
	cout << "  --save-normals <file> : Save normal map\n";
	cout << "  --save-heights <file> : Save height map\n";
	cout << "  --scale-down <int>    : Scale down by factor of 2^n\n";
	cout << "  -q <int>              : JPEG quality (default: 95)\n\n";
	
	cout << "Other options:\n";
	cout << "  -h, --help            : Show this help\n";
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

void averageFilter(float sigma, int w, int h, vector<float> &height, vector<float> &filtered) {

	int window = ceil(3*sigma);
	filtered.resize(w*h);

bool parseArgs(int argc, char *argv[], ExtendedNormalsParameters &config) {
	opterr = 0;
	
	struct option long_options[] = {
		{"flat-radial", required_argument, 0, 1000},
		{"flat-fourier", required_argument, 0, 1001},
		{"flat-blur", required_argument, 0, 1002},
		{"bni-k", required_argument, 0, 1003},
		{"bni-tol", required_argument, 0, 1004},
		{"bni-iter", required_argument, 0, 1005},
		{"assm-error", required_argument, 0, 1006},
		{"save-normals", required_argument, 0, 1007},
		{"save-heights", required_argument, 0, 1008},
		{"scale-down", required_argument, 0, 1009},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};
	
	int c;
	int option_index = 0;
	
	while ((c = getopt_long(argc, argv, "hr:n:s:3:k:i:p:q:", long_options, &option_index)) != -1) {
		switch (c) {
		case 'h':
			help();
			return false;
		case 'r':
			config.input_project = QString(optarg);
			break;
		case 'n':
			config.input_normalmap = QString(optarg);
			break;
		case 's': {
			QString solver_str = QString(optarg).toLower();
			if (solver_str == "l2") config.solver = NORMALS_L2;
			else if (solver_str == "sbl") config.solver = NORMALS_SBL;
			else if (solver_str == "rpca") config.solver = NORMALS_RPCA;
			else {
				cerr << "Unknown solver: " << optarg << endl;
				return false;
			}
			config.generate_normals = true;
			break;
		}
		case '3': {
			QString params = QString(optarg);
			QStringList parts = params.split(':');
			config.dome_radius = parts[0].toDouble();
			if (parts.size() > 1) {
				config.dome_offset = parts[1].toDouble();
			}
			config.use_3d_lights = true;
			break;
		}
		case 'k': {
			QString crop_str = QString(optarg);
			QRegExp rx("(\\d+)x(\\d+)\\+(\\d+)\\+(\\d+)");
			if (rx.exactMatch(crop_str)) {
				config.crop_width = rx.cap(1).toInt();
				config.crop_height = rx.cap(2).toInt();
				config.crop_x = rx.cap(3).toInt();
				config.crop_y = rx.cap(4).toInt();
				config.use_crop = true;
			} else {
				cerr << "Invalid crop format. Use: WxH+X+Y" << endl;
				return false;
			}
			break;
		}
		case 'i': {
			QString method = QString(optarg).toLower();
			if (method == "bni") config.surface_integration = SURFACE_BNI;
			else if (method == "fft") config.surface_integration = SURFACE_FFT;
			else if (method == "assm") config.surface_integration = SURFACE_ASSM;
			else {
				cerr << "Unknown integration method: " << optarg << endl;
				return false;
			}
			break;
		}
		case 'p':
			config.output_ply = QString(optarg);
			break;
		case 'q':
			config.quality = QString(optarg).toInt();
			break;
		case 1000: // --flat-radial
			config.flatMethod = FLAT_RADIAL;
			config.flatPercentage = QString(optarg).toDouble();
			break;
		case 1001: { // --flat-fourier
			QString params = QString(optarg);
			QStringList parts = params.split(':');
			config.flatMethod = FLAT_FOURIER;
			config.fourier_sigma = parts[0].toDouble();
			if (parts.size() > 1) config.fourier_padding = parts[1].toDouble();
			if (parts.size() > 2) config.fourier_exponential = parts[2].toInt() != 0;
			break;
		}
		case 1002: // --flat-blur
			config.flatMethod = FLAT_BLUR;
			config.blur_sigma = QString(optarg).toDouble();
			break;
		case 1003: // --bni-k
			config.bni_k = QString(optarg).toDouble();
			break;
		case 1004: // --bni-tol
			config.bni_tolerance = QString(optarg).toDouble();
			break;
		case 1005: // --bni-iter
			config.bni_max_iterations = QString(optarg).toInt();
			break;
		case 1006: // --assm-error
			config.assm_error = QString(optarg).toDouble();
			break;
		case 1007: // --save-normals
			config.output_normalmap = QString(optarg);
			break;
		case 1008: // --save-heights
			config.output_heightmap = QString(optarg);
			break;
		case 1009: // --scale-down
			config.scale_down = QString(optarg).toInt();
			break;
		case '?':
			cerr << "Unknown option: " << char(optopt) << endl;
			return false;
		}
	}
	
	// Get input folder if not specified as option
	if (config.input_folder.isEmpty() && config.input_project.isEmpty() && config.input_normalmap.isEmpty()) {
		if (optind < argc) {
			config.input_folder = QString(argv[optind]);
		} else {
			cerr << "No input specified" << endl;
			return false;
		}
	}
	
	return true;
}

bool loadNormalMap(const QString &filename, int &width, int &height, std::vector<float> &normals) {
	QImage img(filename);
	if (img.isNull()) {
		cerr << "Failed to load normal map: " << qPrintable(filename) << endl;
		return false;
	}
	
	width = img.width();
	height = img.height();
	normals.resize(width * height * 3);
	
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			QRgb pixel = img.pixel(x, y);
			int idx = (y * width + x) * 3;
			normals[idx] = (qRed(pixel) / 255.0f) * 2.0f - 1.0f;     // X
			normals[idx + 1] = (qGreen(pixel) / 255.0f) * 2.0f - 1.0f; // Y  
			normals[idx + 2] = (qBlue(pixel) / 255.0f) * 2.0f - 1.0f;  // Z
		}
	}
	
	return true;
}

bool generateNormals(const ExtendedNormalsParameters &config, int &width, int &height, std::vector<float> &normals) {
	if (!config.input_project.isEmpty()) {
		// Load from .relight project
		Project project;
		if (!project.load(config.input_project)) {
			cerr << "Failed to load project: " << qPrintable(config.input_project) << endl;
			return false;
		}
		
		// TODO: Implement normal generation from project
		// This would use the same logic as in NormalsTask
		cerr << "Project-based normal generation not yet implemented" << endl;
		return false;
		
	} else if (!config.input_folder.isEmpty()) {
		// Load from folder with .lp file
		ImageSet imageset;
		if (!imageset.load(config.input_folder)) {
			cerr << "Failed to load imageset from: " << qPrintable(config.input_folder) << endl;
			return false;
		}
		
		// TODO: Implement normal generation from imageset
		// This would use RtiBuilder with normal extraction
		cerr << "Folder-based normal generation not yet implemented" << endl;
		return false;
	}
	
	return false;
}

bool flattenNormals(const ExtendedNormalsParameters &config, int width, int height, std::vector<float> &normals) {
	switch (config.flatMethod) {
	case FLAT_RADIAL:
		flattenRadialNormals(width, height, normals, config.flatPercentage);
		break;
	case FLAT_FOURIER:
		flattenFourierNormals(width, height, normals, config.fourier_padding, 
		                     config.fourier_sigma, config.fourier_exponential);
		break;
	case FLAT_BLUR:
		flattenBlurNormals(width, height, normals, config.blur_sigma);
		break;
	case FLAT_NONE:
		// No flattening
		break;
	}
	return true;
}

bool integrateSurface(const ExtendedNormalsParameters &config, int width, int height, 
                     const std::vector<float> &normals, std::vector<float> &heights) {
	heights.resize(width * height);
	
	auto progress_callback = [](QString message, int percent) -> bool {
		cout << qPrintable(message) << " " << percent << "%" << endl;
		return true;
	};
	
	switch (config.surface_integration) {
	case SURFACE_BNI:
		bni_integrate(progress_callback, width, height, 
		             const_cast<std::vector<float>&>(normals), heights,
		             config.bni_k, config.bni_tolerance, config.bni_solver_tolerance,
		             config.bni_max_iterations, config.bni_max_solver_iterations);
		break;
	case SURFACE_FFT:
		fft_integrate(progress_callback, width, height,
		             const_cast<std::vector<float>&>(normals), heights);
		break;
	case SURFACE_ASSM:
		// TODO: Implement ASSM integration
		cerr << "ASSM integration not yet implemented" << endl;
		return false;
	case SURFACE_NONE:
		// No integration
		return true;
	}
	
	return true;
}

void saveNormalMap(const QString &filename, int width, int height, const std::vector<float> &normals) {
	QImage img(width, height, QImage::Format_RGB888);
	
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int idx = (y * width + x) * 3;
			int r = qBound(0, int((normals[idx] + 1.0f) * 127.5f), 255);
			int g = qBound(0, int((normals[idx + 1] + 1.0f) * 127.5f), 255);
			int b = qBound(0, int((normals[idx + 2] + 1.0f) * 127.5f), 255);
			img.setPixel(x, y, qRgb(r, g, b));
		}
	}
	
	img.save(filename);
}

void saveHeightMap(const QString &filename, int width, int height, const std::vector<float> &heights) {
	// Find min/max for normalization
	auto minmax = std::minmax_element(heights.begin(), heights.end());
	float min_height = *minmax.first;
	float max_height = *minmax.second;
	float range = max_height - min_height;
	
	QImage img(width, height, QImage::Format_Grayscale8);
	
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int idx = y * width + x;
			int gray = 0;
			if (range > 0) {
				gray = qBound(0, int(((heights[idx] - min_height) / range) * 255), 255);
			}
			img.setPixel(x, y, qRgb(gray, gray, gray));
		}
	}
	
	img.save(filename);
}

int main(int argc, char *argv[]) {
	QCoreApplication app(argc, argv);
	
	if (argc == 1) {
		help();
		return 1;
	}
	
	ExtendedNormalsParameters config;
	if (!parseArgs(argc, argv, config)) {
		return 1;
	}
	
	int width, height;
	std::vector<float> normals;
	
	// Load or generate normals
	if (!config.input_normalmap.isEmpty()) {
		// Load existing normal map
		if (!loadNormalMap(config.input_normalmap, width, height, normals)) {
			return 1;
		}
	} else if (config.generate_normals) {
		// Generate normals from images
		if (!generateNormals(config, width, height, normals)) {
			return 1;
		}
	} else {
		cerr << "No normal map input or generation specified" << endl;
		return 1;
	}
	
	// Scale down if requested
	for (int i = 0; i < config.scale_down; i++) {
		width /= 2;
		height /= 2;
		// TODO: Implement proper downsampling
	}
	
	// Apply flattening
	if (config.flatMethod != FLAT_NONE) {
		flattenNormals(config, width, height, normals);
	}
	
	// Save normal map if requested
	if (!config.output_normalmap.isEmpty()) {
		saveNormalMap(config.output_normalmap, width, height, normals);
	}
	
	// Integrate surface if requested
	std::vector<float> heights;
	if (config.surface_integration != SURFACE_NONE) {
		if (!integrateSurface(config, width, height, normals, heights)) {
			return 1;
		}
		
		// Save height map if requested
		if (!config.output_heightmap.isEmpty()) {
			saveHeightMap(config.output_heightmap, width, height, heights);
		}
		
		// Save PLY if requested
		if (!config.output_ply.isEmpty()) {
			// TODO: Implement PLY export
			cerr << "PLY export not yet implemented" << endl;
		}
	}
	
	cout << "Processing completed successfully" << endl;
	return 0;
}



		//min = minValue;
		//max = maxValue;
		//cout << "Real min: " << minValue << " percent min: " << min << endl;
		//cout << "Real max: " << maxValue << " percent max: " << max << endl;
		for(int y = 0; y < H; y++) {
			for(int x = 0; x < W; x++) {
				float v = 2*(filtered[x + y*W] - min)/(max - min);
				if(v > 1)	 v = 1;
				int l = (int)(v*255.0f);
				img.setPixel(x, y, qRgb(l, l, l));
			}
		}

		img.save(QString("heightmap_%1_%2.jpg").arg(filter_sigma).arg(low_percentile));
	}
	return 0;
#endif
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
        cout<<"print"<<endl;
        savePly(ply, w, h, height_map);
		saveBlob(ply + ".blob", w, h, height_map);
	}
}
