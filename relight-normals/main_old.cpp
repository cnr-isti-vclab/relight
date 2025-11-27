#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QImage>

#include "../src/normals/normalstask.h"
#include "../src/project.h"
#include "../src/imageset.h"
#include "../src/getopt.h"

#include <iostream>
#include <getopt.h>

extern int opterr;

using namespace std;

struct CLINormalsParameters : public NormalsParameters {
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
	cout << "  --flat-fourier <sigma> : Fourier flattening with sigma\n";
	cout << "  --flat-blur <sigma>   : Blur flattening with sigma (default: 10)\n\n";
	
	cout << "Integration options:\n";
	cout << "  -i <method>           : Integration method: bni, fft, assm (default: none)\n";
	cout << "  --bni-k <float>       : BNI discontinuity parameter (default: 2.0)\n";
	cout << "  --assm-error <float>  : ASSM target error (default: 0.1)\n\n";
	
	cout << "Output options:\n";
	cout << "  -o <output>           : Output file (without extension)\n";
	cout << "  --save-normals <file> : Save normal map\n";
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

bool parseArgs(int argc, char *argv[], CLINormalsParameters &config) {
	opterr = 0;
	
	struct option long_options[] = {
		{"flat-radial", required_argument, 0, 1000},
		{"flat-fourier", required_argument, 0, 1001},
		{"flat-blur", required_argument, 0, 1002},
		{"bni-k", required_argument, 0, 1003},
		{"assm-error", required_argument, 0, 1006},
		{"save-normals", required_argument, 0, 1007},
		{"scale-down", required_argument, 0, 1009},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};
	
	int c;
	int option_index = 0;
	
	while ((c = getopt_long(argc, argv, "hr:n:s:3:k:i:o:q:", long_options, &option_index)) != -1) {
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
		case 'o':
			config.path = QString(optarg);
			break;
		case 'q':
			config.quality = QString(optarg).toInt();
			break;
		case 1000: // --flat-radial
			config.flatMethod = FLAT_RADIAL;
			config.flatPercentage = QString(optarg).toDouble();
			break;
		case 1001: // --flat-fourier
			config.flatMethod = FLAT_FOURIER;
			config.flatPercentage = QString(optarg).toDouble();
			break;
		case 1002: // --flat-blur
			config.flatMethod = FLAT_BLUR;
			config.blurPercentage = QString(optarg).toDouble();
			break;
		case 1003: // --bni-k
			config.bni_k = QString(optarg).toDouble();
			break;
		case 1006: // --assm-error
			config.assm_error = QString(optarg).toDouble();
			break;
		case 1007: // --save-normals
			config.output_normalmap = QString(optarg);
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

int main(int argc, char *argv[]) {
	QCoreApplication app(argc, argv);
	
	if (argc == 1) {
		help();
		return 1;
	}
	
	CLINormalsParameters config;
	if (!parseArgs(argc, argv, config)) {
		return 1;
	}
	
	// Set up default output path if not specified
	if (config.path.isEmpty()) {
		if (!config.input_normalmap.isEmpty()) {
			QFileInfo info(config.input_normalmap);
			config.path = info.absolutePath() + "/" + info.baseName() + "_processed.png";
		} else if (!config.input_project.isEmpty()) {
			QFileInfo info(config.input_project);
			config.path = info.absolutePath() + "/" + info.baseName() + "_normals.png";
		} else if (!config.input_folder.isEmpty()) {
			QDir dir(config.input_folder);
			config.path = dir.absolutePath() + "/normals.png";
		}
	}
	
	// Create and configure the task
	NormalsTask task;
	
	// Set parameters
	NormalsParameters params = config;
	if (!config.input_normalmap.isEmpty()) {
		params.compute = false;
		params.input_path = config.input_normalmap;
	} else {
		params.compute = true;
	}
	
	// Set surface dimensions for scaling
	if (config.scale_down > 0) {
		// Will be applied in the task
		params.surface_width = 0;  // Will be calculated based on scale_down
		params.surface_height = 0;
	}
	
	task.setParameters(params);
	task.output = config.path;
	
	try {
		if (!config.input_project.isEmpty()) {
			// Load from project
			Project project;
			if (!project.load(config.input_project)) {
				cerr << "Failed to load project: " << qPrintable(config.input_project) << endl;
				return 1;
			}
			
			// Apply crop if specified
			if (config.use_crop) {
				project.crop.width = config.crop_width;
				project.crop.height = config.crop_height;
				project.crop.left = config.crop_x;
				project.crop.top = config.crop_y;
			}
			
			task.initFromProject(project);
			
		} else if (!config.input_folder.isEmpty()) {
			// Load from folder
			Dome dome;
			if (config.use_3d_lights) {
				dome.radius = config.dome_radius;
				if (config.dome_offset != 0.0) {
					dome.center[2] = config.dome_offset;
				}
			}
			
			Crop crop;
			if (config.use_crop) {
				crop.width = config.crop_width;
				crop.height = config.crop_height;
				crop.left = config.crop_x;
				crop.top = config.crop_y;
			}
			
			task.initFromFolder(config.input_folder.toLocal8Bit().data(), dome, crop);
		}
		
		// Connect progress signal to show progress
		QObject::connect(&task, &NormalsTask::progress, [](QString message, int percent) {
			cout << qPrintable(message) << " " << percent << "%" << endl;
		});
		
		// Run the task
		cout << "Starting normal processing..." << endl;
		task.run();
		
		if (task.status == Task::DONE) {
			cout << "Processing completed successfully" << endl;
			cout << "Output saved to: " << qPrintable(config.path) << endl;
			
			// Save additional outputs if requested
			if (!config.output_normalmap.isEmpty() && config.output_normalmap != config.path) {
				QFile::copy(config.path, config.output_normalmap);
				cout << "Normal map saved to: " << qPrintable(config.output_normalmap) << endl;
			}
			
			return 0;
		} else {
			cerr << "Processing failed: " << qPrintable(task.error) << endl;
			return 1;
		}
		
	} catch (const std::exception& e) {
		cerr << "Error: " << e.what() << endl;
		return 1;
	} catch (...) {
		cerr << "Unknown error occurred" << endl;
		return 1;
	}
}
