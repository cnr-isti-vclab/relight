#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringList>
#include <QImage>
#include <QFileInfo>

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
	QString input_path;
	
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
	double scale_down = 0.0;
};

void help() {
	cout << "Relight Normals - Generate normal maps and integrate surfaces\n\n";
	cout << "Usage: relight-normals [options] <input>\n\n";
	
	cout << "Input:\n";
	cout << "  <folder>              : Process folder with .lp file\n";
	cout << "  <project.relight>     : Process .relight project file\n";
	cout << "  <normalmap.png/jpg>   : Process existing normal map\n\n";
	
	cout << "Normal generation options (for folder/project input):\n";
	cout << "  -s <solver>           : Normal solver: l2 (default), sbl*, rpca* (*not implemented yet)\n";
	cout << "  -3 <radius[:offset]>  : 3D light positions, dome radius and optional offset\n";
	cout << "  -k <w>x<h>+<x>+<y>    : Crop to width x height at offset x,y\n\n";
	
	cout << "Flattening options:\n";
	cout << "  --flat-radial         : Radial flattening\n";
	cout << "  --flat-fourier <sigma> : Fourier flattening with sigma\n";
	cout << "  --flat-blur <sigma>   : Blur flattening with sigma (default: 10)\n\n";
	
	cout << "Integration options:\n";
	cout << "  -i <method>           : Integration method: bni, fft, assm (default: none)\n";
	cout << "  --bni-k <float>       : BNI discontinuity parameter (default: 2.0)\n";
	cout << "  --assm-error <float>  : ASSM target error (default: 0.1)\n\n";
	
	cout << "Output options:\n";
	cout << "  -o <output>           : Output file (without extension)\n";
	cout << "  --save-normals <file> : Save normal map\n";
	cout << "  --scale-down <float>  : Scale down by factor\n";
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
		{"flat-radial", no_argument, 0, 1000},
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
	
	while ((c = getopt_long(argc, argv, "hs:3:k:i:o:q:", long_options, &option_index)) != -1) {
		switch (c) {
		case 'h':
			help();
			return false;
		case 's': {
			QString solver_str = QString(optarg).toLower();
			if (solver_str == "l2") config.solver = NORMALS_L2;
			else if (solver_str == "sbl") {
				cerr << "SBL solver is not implemented yet" << endl;
				return false;
			}
			else if (solver_str == "rpca") {
				cerr << "RPCA solver is not implemented yet" << endl;
				return false;
			}
			else {
				cerr << "Unknown solver: " << optarg << ". Available: l2" << endl;
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
			QRegularExpression rx("(\\d+)x(\\d+)\\+(\\d+)\\+(\\d+)");
			QRegularExpressionMatch match = rx.match(crop_str);
			if (match.hasMatch()) {
				config.crop_width = match.captured(1).toInt();
				config.crop_height = match.captured(2).toInt();
				config.crop_x = match.captured(3).toInt();
				config.crop_y = match.captured(4).toInt();
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
			config.scale_down = QString(optarg).toDouble();
			break;
		case '?':
			cerr << "Unknown option: " << char(optopt) << endl;
			return false;
		}
	}
	
	// Get input path
	if (optind < argc) {
		config.input_path = QString(argv[optind]);
	} else {
		cerr << "No input specified" << endl;
		return false;
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
		QFileInfo info(config.input_path);
		if (info.suffix().toLower() == "relight") {
			config.path = info.absolutePath() + "/" + info.baseName() + "_normals.png";
		} else if (info.suffix().toLower() == "png" || info.suffix().toLower() == "jpg" || info.suffix().toLower() == "jpeg") {
			config.path = info.absolutePath() + "/" + info.baseName() + "_processed.png";
		} else if (info.isDir()) {
			// It's a directory
			QDir dir(config.input_path);
			config.path = dir.absolutePath() + "/normals.png";
		} else {
			cerr << "Unknown input type: " << qPrintable(config.input_path) << endl;
			return 1;
		}
	}
	
	// Create and configure the task
	NormalsTask task;
	
	// Set parameters based on input type
	QFileInfo info(config.input_path);
	NormalsParameters params = config;
	
	if (info.suffix().toLower() == "png" || info.suffix().toLower() == "jpg" || info.suffix().toLower() == "jpeg") {
		// Processing existing normal map
		params.compute = false;
		params.input_path = config.input_path;
	} else {
		// Generating normals from folder or project
		params.compute = true;
	}
	
	task.setParameters(params);
	task.output = config.path;
	
	try {
		if (info.suffix().toLower() == "relight") {
			// Load from project file
			Project project;
			try {
				project.load(config.input_path);
			} catch (...) {
				cerr << "Failed to load project: " << qPrintable(config.input_path) << endl;
				return 1;
			}
			
			// Apply crop if specified
			if (config.use_crop) {
				project.crop.setWidth(config.crop_width);
				project.crop.setHeight(config.crop_height);
				project.crop.setLeft(config.crop_x);
				project.crop.setTop(config.crop_y);
				project.crop.angle = 0.0f;
			}
			
			task.initFromProject(project);
			
		} else if (info.suffix().toLower() == "png" || info.suffix().toLower() == "jpg" || info.suffix().toLower() == "jpeg") {
			// Processing existing normal map - no initialization needed
			
		} else if (info.isDir()) {
			// Load from directory

			QDir dir = info.dir();
			QStringList lp_ext;
			lp_ext << "*.lp";
			QStringList lps = dir.entryList(lp_ext);
			if(lps.size() == 0)
				throw QString("Could not find a .lp file in the folder");

			Dome dome;
			if (config.use_3d_lights) {
				dome.domeDiameter = config.dome_radius * 2.0;  // diameter from radius
				if (config.dome_offset != 0.0) {
					dome.verticalOffset = config.dome_offset;
				}
			}
			dome.parseLP(dir.filePath(lps[0]));
			
			Crop crop;
			if (config.use_crop) {
				crop.setWidth(config.crop_width);
				crop.setHeight(config.crop_height);
				crop.setLeft(config.crop_x);
				crop.setTop(config.crop_y);
			}
			
			task.initFromFolder(config.input_path.toLocal8Bit().data(), dome, crop);
		} else {
			cerr << "Unknown input type: " << qPrintable(config.input_path) << endl;
			return 1;
		}
		
		// Apply scaling after initialization when we know the actual dimensions
		if (config.scale_down > 0.0) {
			task.parameters.surface_width = (int)(task.imageset.image_width / config.scale_down);
			task.parameters.surface_height = (int)(task.imageset.image_height / config.scale_down);
		}
		
		// Connect progress signal to show progress
		QObject::connect(&task, &NormalsTask::progress, [](QString message, int percent) {
			cout << qPrintable(message) << " " << percent << "%" << endl;
		});
		
		// Run the task
		cout << "Starting normal processing..." << endl;
		task.run();
		
		if (task.status == Task::DONE) {
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
