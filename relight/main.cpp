#include "rtibuilder.h"
#include "../src/legacy_rti.h"

#include "../src/getopt.h"

#include <QDir>

#include <iostream>
#include <string>
using namespace std;

//TODO concentric map e sampling rate (1 in N)
// https://www.compuphase.com/cmetric.htm
//metric difference between colors

//adaptive base reflectance
void help() {
	//cout << "\t-f <format>: flat\n";
	cout << "Usage: relight [-mrdqp]<input folder> [output folder]\n\n";
	cout << "\tinput folder containing a .lp with number of photos and light directions\n";
	cout << "\toptional output folder (default ./)\n\n";

	cout << "\t-b <basis>: rbf(default), ptm, lptm, hsh, yrbf, bilinear, dmd\n";
	cout << "\t-m <int>  : number of materials (default 8)\n";
	cout << "\t-r <int>  : side of the basis function (default 8, 0 means rbf interpolation)\n";
	cout << "\t-q <int>  : jpeg quality (default: 90)\n";
	cout << "\t-p <int>  : number of planes (default: 9)\n";
	cout << "\t-s <int>  : sampling rate for pca (default 4)\n";
	cout << "\t-S <int>  : sigma in rgf gaussian interpolation default 0.125 (~100 img)\n";
	cout << "\t-c <float>: coeff quantization (to test!) default 1.5\n";
	cout << "\t-C        : apply chroma subsampling \n";
	cout << "\t-e        : evaluate reconstruction error (default: false)\n";
	cout << "\t-E <int>  : evaluate error on a single image (but remove it for fitting)\n";
	cout << "\t-y <int>  : number of Y planes in YCC\n\n";
}

int convertRTI(const char *file, const char *output, int quality);

int main(int argc, char *argv[]) {

	if(argc == 1) {
		help();
		return 0;
	}


	RtiBuilder builder;
	int quality = 95;
	bool evaluate_error = false;
	int reference_image = -1; //image used for error reference

	opterr = 0;
	char c;
	while ((c  = getopt (argc, argv, "hm:r:d:q:p:s:c:reE:b:y:S:C")) != -1)
		switch (c)
		{
		case 'h':
			help();
			break;
		case 'm':
			builder.nmaterials = (uint32_t)atoi(optarg);
			break;
		case 'r': {
				int res = atoi(optarg);
				builder.resolution = res = atoi(optarg);
				if(res < 0 || res == 1 || res == 2 || res > 10) {
					cerr << "Invalid resolution (must be 0 or >= 2 && <= 10)\n";
					return 1;
				}
			}
			break;
		case 'b': {
			string b = optarg;
			if(b == "rbf") {
				builder.type = RtiBuilder::RBF;
				builder.colorspace = RtiBuilder::MRGB;

			} else if(b == "bilinear") {
				builder.type = RtiBuilder::BILINEAR;
				builder.colorspace = RtiBuilder::MRGB;

			} else if(b == "hsh") {
				builder.type = RtiBuilder::HSH;
				builder.colorspace = RtiBuilder::RGB;

			} else if(b == "lhsh") {
				builder.type = RtiBuilder::HSH;
				builder.colorspace = RtiBuilder::LRGB;

			} else if(b == "ptm") {
				builder.type = RtiBuilder::PTM;
				builder.colorspace = RtiBuilder::RGB;

			} else if(b == "lptm") {
				builder.type = RtiBuilder::PTM;
				builder.colorspace = RtiBuilder::LRGB;

			} else if(b == "yrbf") {
				builder.type = RtiBuilder::RBF;
				builder.colorspace = RtiBuilder::MYCC;

			} else if(b == "yptm") {
				builder.type = RtiBuilder::PTM;
				builder.colorspace = RtiBuilder::YCC;
				
			} else if(b == "yhsh") {
				builder.colorspace = RtiBuilder::YCC;
				builder.type = RtiBuilder::HSH;
			} else if(b == "dmd") {
				builder.colorspace = RtiBuilder::RGB;
				builder.type = RtiBuilder::DMD;
			} else {
				cerr << "Unknown basis type: " << optarg << " (pick yrbf, rbf, bilinear, hsh, dmd or ptm)\n";
				return 1;
			}
		}
			break;
/*		case 'd':
			encoder.distortion = atof(optarg);
			break; */
		case 'e':
			evaluate_error = true;
			break;
		case 'E':
			reference_image = atoi(optarg);
			break;
		case 'q':
			quality = atoi(optarg);
			break;
		case 'p':
			builder.nplanes = (uint32_t)atoi(optarg);
			break;
		case 'y':
			builder.yccplanes[0] = (uint32_t)atoi(optarg);
			break;
		case 's':
			builder.samplingrate = (uint32_t)atoi(optarg);
			break;
		case 'S': {
			float sigma = (float)atof(optarg);
			if(sigma > 0)
				builder.sigma = sigma;
			break;
		}
		case 'C':
			builder.chromasubsampling = true;
			break;

		case 'c':
			builder.rangescale = (float)atof(optarg);
			break;

		case '?':
			cerr << "Option " << (char)optopt << " requires an argument" << endl;
			if (isprint (optopt))
				cerr << "Unknown option " << (char)optopt << endl;
			else
				cerr << "Unknown option character" << endl;
			return 1;
		default:
			cerr << "Unknown error" << endl;
			return 1;
		}

	if(optind == argc) {
		cerr << "Too few arguments" << endl;
		help();
		return 1;
	}
	if(optind + 2 < argc) {
		cerr << "Too many arguments" << endl;
		help();
		return 1;
	}
	if( builder.colorspace == Rti::MYCC) {
		if(builder.yccplanes[0] == 0) {
			cerr << "Y nplanes in mycc must be specified (-y)" << endl;
			return 1;
		}
		builder.yccplanes[1] = builder.yccplanes[2] = (builder.nplanes - builder.yccplanes[0])/2;
		builder.nplanes = builder.yccplanes[0] + 2*builder.yccplanes[1];
	}

	std::string input = argv[optind++];
	std::string output("./");
	if(optind < argc)
		output = argv[optind++];
	
	QFileInfo info(input.c_str());
	if(info.isFile())
		return convertRTI(input.c_str(), output.c_str(), quality);
	
	
	if(!builder.init(input, reference_image)) {
		cerr << builder.error << endl;
		return 1;
	}

	int size = builder.save(output, quality);
	if(size == 0) {
		cerr << "Failed saving: " << builder.error << endl;
		return 1;
	}

	if(evaluate_error) {
		Rti rti;
		if(!rti.load(output.c_str())) {
			cerr << "Failed loading rti: " << output << endl;
			return 1;
		}
		QDir out(output.c_str());
		double mse = Rti::evaluateError(builder.imageset, rti, out.filePath("error.png"), reference_image);
		double psnr = 20*log10(255.0) - 10*log10(mse);
		mse = sqrt(mse);
		
		if(psnr == 0.0f) {
			cerr << "Failed reloading rti: " << builder.error << endl;
		}
		//type, colorspace, nplanes, nmaterials, ny
		map<Rti::Type, string> types = { { Rti::PTM, "ptm" }, {Rti::HSH, "hsh"}, {Rti::RBF, "rbf"}, { Rti::BILINEAR, "bilinear"} };
		map<Rti::ColorSpace, string> colorspaces = { { Rti::RGB, "rgb"}, { Rti::LRGB, "lrgb" }, { Rti::YCC, "ycc"}, { Rti::MRGB, "mrgb"}, { Rti::MYCC, "mycc" } };

		cout << output << "," << types[builder.type] << "," << colorspaces[builder.colorspace] << ","
			<< builder.nplanes << ","<< builder.nmaterials << "," << builder.yccplanes[0] << "," << size << "," << psnr << "," << mse << endl;
	}
	return 0;
}

int convertRTI(const char *file, const char *output, int quality) {
	LRti lrti;
	if(!lrti.load(file))
		return 1;

	RtiBuilder rti;
	rti.width = lrti.width;
	rti.height = lrti.height;
	rti.chromasubsampling = lrti.chromasubsampled;
	switch(lrti.type) {
	case LRti::UNKNOWN:
		cerr << "Unknown RTI type!\n";
		return 1;
	case LRti::PTM_LRGB:
		rti.type = Rti::PTM;
		rti.colorspace = Rti::LRGB;
		rti.nplanes = 9;
		break;
	case LRti::PTM_RGB:
		rti.type = Rti::PTM;
		rti.colorspace = Rti::RGB;
		rti.nplanes = 18;
		break;
	case LRti::HSH:
		rti.type = Rti::HSH;
		rti.colorspace = Rti::RGB;
		rti.nplanes = lrti.scale.size()*3;
		break;
	}

	vector<int> order = { 5,3,4, 0,2,1};
	
	if(lrti.type == LRti::PTM_LRGB) {
		rti.scale = {1, 1, 1}; //rgb coeff in lptm have no scale or bias;
		rti.bias = {0, 0, 0};
		for(int i = 0; i < 6; i++) {
			rti.scale.push_back(lrti.scale[order[i]]);
			rti.bias.push_back((float)lrti.bias[order[i]]);
		}

	} else if(lrti.type == LRti::PTM_RGB){
		for(int i = 0; i < 6; i++) {
			for(int k = 0; k < 3; k++) {
				rti.scale.push_back(lrti.scale[order[i]]);
				rti.bias.push_back(lrti.bias[order[i]]);
			}
		}
	} else {
		rti.scale.resize(lrti.scale.size()*3);
		rti.bias.resize(lrti.scale.size()*3);
		for(size_t i = 0; i < lrti.scale.size(); i++) {
			rti.scale[i*3] = rti.scale[i*3+1] = rti.scale[i*3+2] = lrti.scale[i];
			rti.bias[i*3] = rti.bias[i*3+1] = rti.bias[i*3+2] = lrti.bias[i];
			
		}
	}
	
	QDir dir(output);
	if(!dir.exists()) {
		QDir here("./");
		if(!here.mkdir(output)) {
			cerr << "Could not create output dir.\n";
			return 1;
		}
	}
	
	rti.saveJSON(dir, quality);
	for(uint32_t p = 0; p < rti.nplanes; p += 3) {
		lrti.encodeJPEG(p, quality, dir.filePath("plane_%1.jpg").arg(p/3).toStdString().c_str());
	}
	//time to save the JPG
	
	
	return 0;
}
