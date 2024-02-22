#include "rtibuilder.h"
//#include "../src/legacy_rti.h"

#include "../src/getopt.h"
extern int opterr;

#include <QDir>
#include <QImage>
//#include <QJsonDocument>
//#include <QJsonObject>
//#include <QJsonArray>
#include <QElapsedTimer>

#include <iostream>
#include <string>
using namespace std;

//TODO concentric map e sampling rate (1 in N)
// https://www.compuphase.com/cmetric.htm
//metric difference between colors

//adaptive base reflectance
void help() {
	cout << "Create an RTI from a set of images and a set of light directions (.lp) in a folder.\n";
	cout << "It is also possible to convert from .ptm or .rti to relight format and viceversa.\n\n";
	cout << "Usage: relight-cli [-bpqy3PnmMwkrsSRBcCeEv]<input folder> [output folder]\n\n";
	cout << "       relight-cli [-q] <input.ptm|.rti> [output folder]\n\n";
	cout << "       relight-cli [-q] <input.json> [output.ptm]\n\n";
    cout << "\tinput folder containing a .lp with number of photos and light directions\n";
    cout << "\toptional output folder (default ./)\n\n";
    cout << "\t-b <basis>: rbf(default), ptm, lptm, hsh, yrbf, bilinear\n";
    cout << "\t-p <int>  : number of planes (default: 9)\n";
    cout << "\t-q <int>  : jpeg quality (default: 95)\n";
    cout << "\t-y <int>  : number of Y planes in YCC\n\n";
	cout << "\t-3 <radius[:offset]>: 3d light positions processing, ratio diameter_dome/image_width\n               and optionally vertical offset of the center of the sphere to the surface.\n";

	cout << "\t-P <pixel size in MM>: this number is saved in .json output\n";
    //	cout << "\t-m <int>  : number of materials (default 8)\n";
	cout << "\t-n        : extract normals\n";
	cout << "\t-m        : extract mean image\n";
	cout << "\t-M        : extract median image (7/8th quantile) \n";

	cout << "\t-w        : number of workers (default 8)\n";
	cout << "\t-k <int>x<int>+<int>+<int>: Cropping extracts only the widthxheight+offx+offy part\n";

    cout << "\nIgnore exotic parameters below here\n\n";
	cout << "\n-H        : fix overexposure in ptm and hsh due to bad sampling\n";
    cout << "\t-r <int>  : side of the basis function (default 8, 0 means rbf interpolation)\n";
	cout << "\t-s <int>  : sampling RAM for pca  in MB (default 500MB)\n";
    cout << "\t-S <float>: sigma in rgf gaussian interpolation default 0.125 (~100 img)\n";
    cout << "\t-R <float>: regularization coeff for bilinear default 0.1\n";
    cout << "\t-B <float>: range compress bits for planes (default 0.0) 1.0 means compress\n";
    cout << "\t-c <float>: coeff quantization (to test!) default 1.5\n";
    cout << "\t-C        : apply chroma subsampling \n";
    cout << "\t-e        : evaluate reconstruction error (default: false)\n";
    cout << "\t-E <int>  : evaluate error on a single image (but remove it for fitting)\n";

    cout << "\n\nTesting options, will use the input folder as an RTI source: \n";

    cout << "\t-D <path> : directory to store rebuilt images\n";
    cout << "\t-L <x:y:z> : reconstruct only one image from light parameters, output is the filename\n";
	cout << "\t-v : verbose, prints progress info\n";
}

//convert PTM into relight format
int convertRTI(const char *file, const char *output, int quality);

//converts relight into PTM format
int convertToRTI(const char *file, const char *output);

void test(std::string input, std::string output,  Vector3f light) {

	Rti rti;
    if(!rti.load(input.c_str())) {
        cerr << "Failed loading rti: " << input << " !\n" << endl;
        return;
    }

    light = light / light.norm();
 //   rti.render(light[0], light[1], buffer.data());
    QImage img(rti.width, rti.height, QImage::Format_RGBA8888);
    rti.render(light[0], light[1], img.bits(), 4);
    img.save(output.c_str());
}

bool progress(QString str, int n) {
	static QString previous = "";
	if(previous == str) cout << '\r';
	else if(previous != "")
		cout << "\n";
	cout << qPrintable(str) << " %" << n << std::flush;
	previous = str;
	return true;
}

int main(int argc, char *argv[]) {
	Rti rti1;
	rti1.lightWeightsSh(0.3, 0.2);
    if(argc == 1) {
        help();
        return 0;
    }

    RtiBuilder builder;
    int quality = 95;
    bool evaluate_error = false;
    QString redrawdir;
    bool relighted = false;
    Vector3f light;
	bool verbose = true;
	bool histogram_fix = false;

	opterr = 0;
    char c;
	while ((c  = getopt (argc, argv, "hmMn3:r:d:q:p:s:c:reE:b:y:S:R:CD:B:L:k:v")) != -1)
        switch (c)
        {
        case 'h':
            help();
            break;
        case 'n':
            builder.savenormals = true;
            break;
        case 'm':
            builder.savemeans = true;
            break;
        case 'M':
            builder.savemedians = true;
            break;

            //	builder.nmaterials = (uint32_t)atoi(optarg);
            //	break;
        case 'r': {
            int res = atoi(optarg);
            builder.resolution = res = atoi(optarg);
            if(res < 0 || res == 1 || res == 2 || res > 20) {
                cerr << "Invalid resolution (must be 0 or >= 2 && <= 20)!\n" << endl;
                return 1;
            }
		}
			break;
		case 'P':
			builder.pixelSize = atof(optarg);
			if(builder.pixelSize <= 0) {
				cerr << "Invalidi parameter pixelSize (-p): " << optarg << endl;
				return 1;
			}
            break;
        case 'b': {
            string b = optarg;
            if(b == "rbf") {
                builder.type = RtiBuilder::RBF;
                builder.colorspace = RtiBuilder::MRGB;

            } else if(b == "bilinear" || b == "bln") {
                builder.type = RtiBuilder::BILINEAR;
                builder.colorspace = RtiBuilder::MRGB;

            } else if(b == "hsh") {
                builder.type = RtiBuilder::HSH;
                builder.colorspace = RtiBuilder::RGB;

			} else if(b == "sh") {
				builder.type = RtiBuilder::SH;
				builder.colorspace = RtiBuilder::RGB;

			} else if(b == "h") {
				builder.type = RtiBuilder::H;
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

            } else if(b == "ybilinear" || b == "ybln") {
                builder.type = RtiBuilder::BILINEAR;
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
                cerr << "Unknown basis type: " << optarg << " (pick rbf, ptm, lptm, hsh, yrbf or bilinear!)\n" << endl;
                return 1;
            }
        }
            break;
			/*		case 'd':c
            encoder.distortion = atof(optarg);
            break; */
		case '3': { //assume lights positionals. (0, 0) is in the center of the image, (might add these values), and unit is image width
			builder.imageset.light3d = true;
			builder.imageset.dome_radius = float(atof(optarg));
			QString params(optarg);
			if(params.contains(':')) {
				builder.imageset.vertical_offset = params.split(':')[1].toDouble();
			}
		}
		break;
		case 'w':
			builder.nworkers = std::min(atoi(optarg), 1);
			break;
        case 'e':
            evaluate_error = true;
            break;
        case 'E':
            evaluate_error = true;
            builder.skip_image = atoi(optarg);
            break;
        case 'q':
            quality = atoi(optarg);
            break;
        case 'p':
            builder.nplanes = uint32_t(atoi(optarg));
            break;
        case 'y':
            builder.yccplanes[0] = uint32_t(atoi(optarg));
            break;
        case 's':
			builder.samplingram = uint32_t(atoi(optarg));
            break;
        case 'S': {
            float sigma = float(atof(optarg));
            if(sigma > 0)
                builder.sigma = sigma;
            break;
        }
		case 'k': {
			QString c(optarg);
			QStringList c1 = c.split('x');
			builder.crop[2] = c1[0].toInt();
			QStringList c2 = c1[1].split('+');
			builder.crop[3] = c2[0].toInt();
			if(c2.size() > 1)
				builder.crop[0] = c2[1].toInt();
			if(c2.size() > 2)
				builder.crop[1] = c2[2].toInt();
			if(verbose) {
				cout << "Cropping: X: " << builder.crop[0] << " Y: " << builder.crop[1]
					 << " Width: " << builder.crop[2] << " Height: " << builder.crop[3] << endl;
			}
			break;
		}
		case 'H': {
			builder.histogram_fix = true;
			break;
		}
        case 'R': {
            float reg = float(atof(optarg));
            if(reg > 0)
                builder.regularization = reg;
            break;
        }
        case 'B': {
            float compress = float(atof(optarg));
            if(compress >= 0.0f && compress <= 1.0f)
                builder.rangecompress = compress;
            else {
                cerr << "Range compression must be between 0 and 1!\n" << endl;
                return 1;
            }
            break;
        }
        case 'C':
            builder.chromasubsampling = true;
            break;

        case 'c':
            builder.rangescale = float(atof(optarg));
            break;
        case 'D':
            redrawdir = optarg;
            break;
        case 'L': {
            QStringList par = QString(optarg).split(':');
            if(par.size() != 3) {
                cerr << "Invalid parameters expecting it in format x:y:z \n";
                return 1;
            }
            light[0] = par[0].toFloat();
            light[1] = par[1].toFloat();
            light[2] = par[2].toFloat();
			light = light/light.norm();
            relighted = true;
            break;
        }
		case 'v':
			verbose = true;
			break;
        case '?':
            cerr << "Option " << char(optopt) << " requires an argument!\n" << endl;
            if (isprint (optopt))
                cerr << "Unknown option " << char(optopt) << " !\n" << endl;
            else
                cerr << "Unknown option character!\n" << endl;
            return 1;
        default:
            cerr << "Unknown error!\n" << endl;
            return 1;
        }

    if(optind == argc) {
        cerr << "Too few arguments!\n" << endl;
        help();
        return 1;
    }
    if(optind + 2 < argc) {
        cerr << "Too many arguments!\n" << endl;
        help();
        return 1;
    }

	if(builder.imageset.light3d == true && builder.type == RtiBuilder::RBF) {
		cerr << "RBF basis do not support positional lights (for the moment)\n";
		return 1;
	}


    if( builder.colorspace == Rti::MYCC) {
        if(builder.yccplanes[0] == 0) {
			cerr << "Y nplanes in mycc must be specified (-y)!\n";
            return 1;
        }
		if(builder.nplanes % 3 != 0) {
			cerr << "Number of planes should be a multiple of 3 (9, 18, 21 etc)\n";
			return 1;
		}
		if((builder.nplanes - builder.yccplanes[0]) % 2 != 0) {
			cerr << "Total planes (-p) - luma planes (-y) should be an even number.\n";
			return 1;
		}
        builder.yccplanes[1] = builder.yccplanes[2] = (builder.nplanes - builder.yccplanes[0])/2;
        builder.nplanes = builder.yccplanes[0] + 2*builder.yccplanes[1];

    }

    std::string input = argv[optind++];
    std::string output("./");
    if(optind < argc)
        output = argv[optind++];

    if(relighted) {
		if(redrawdir.isNull()) {
			cerr << "Specify an output image filename using -D option\n" << endl;
			return -1;
		}
		test(input, redrawdir.toStdString(), light);
        return 1;
    }

	std::function<bool(QString stage, int percent)> *callback = nullptr;

	//bool (*callback)(std::string stage, int percent) = nullptr;
	if(verbose) {
		callback = new std::function<bool(QString stage, int percent)>();
		*callback = [](QString stage, int percent)->bool{ return progress(stage, percent); };
	}

	QElapsedTimer timer;
	timer.start();

	QFileInfo info(input.c_str());
	if(info.isFile()) {
		if(info.suffix() == "relight") {
			if(!builder.initFromProject(input, callback)) {
				cerr << builder.error << "\n" << endl;
				return 1;
			}
		} if(info.suffix() == "json") {
			return convertToRTI(input.c_str(), output.c_str());
		} else if(info.suffix() == "rti" || info.suffix() == "ptm") {
			try {
				return convertRTI(input.c_str(), output.c_str(), quality);
			} catch(QString error) {
				cerr << qPrintable(error) << endl;
				return 1;
			}
		} else {
			cerr << "Input parameter (" << input << ") is an unknown type, relight-cli can process .relight, .json, .rti or .ptm files" << endl;
			return 1;
		}
	} else {
		if(!builder.initFromFolder(input, callback)) {
			cerr << builder.error << " !\n" << endl;
			return 1;
		}
	}

    int size = builder.save(output, quality);
    if(size == 0) {
        cerr << "Failed saving: " << builder.error << " !\n" << endl;
        return 1;
	}

	int time = timer.restart();
	if(time < 10000)
		cout << "\nDone in: " << time << "ms" << endl;
	else
		cout << "\nDone in: " << time/1000 << "s" << endl;

    if(redrawdir.size()) {
        Rti rti;
        if(!rti.load(output.c_str())) {
            cerr << "Failed loading rti: " << output << " !\n" << endl;
            return 1;
        }

        QDir dir(redrawdir);
        if(!dir.exists()) {
            cerr << "Directory for redraw not found!\n" << endl;
            return 1;
        }
		for(size_t i = 0; i < builder.lights.size(); i++) {
			Vector3f &rlight = builder.lights[i];
          //  rti.render(rlight[0], rlight[1], buffer.data());

            QImage img(rti.width, rti.height, QImage::Format_RGBA8888);
            rti.render(rlight[0], rlight[1], img.bits(), 4);
            img.save(dir.filePath( builder.imageset.images[i]));
        }
    }

    if(evaluate_error) {
        Rti rti;
        if(!rti.load(output.c_str())) {
            cerr << "Failed loading rti: " << output << " !\n" << endl;
            return 1;
        }

        map<Rti::Type, string> types = { { Rti::PTM, "ptm" }, {Rti::HSH, "hsh"}, {Rti::RBF, "rbf"}, { Rti::BILINEAR, "bilinear"} };
        map<Rti::ColorSpace, string> colorspaces = { { Rti::RGB, "rgb"}, { Rti::LRGB, "lrgb" }, { Rti::YCC, "ycc"}, { Rti::MRGB, "mrgb"}, { Rti::MYCC, "mycc" } };


        if(builder.skip_image == -1) {
            double totmse = 0.0;
			for(size_t i = 0; i < builder.lights.size(); i++) {
                double mse = Rti::evaluateError(builder.imageset, rti, QString(), i);
                totmse += mse;
//                double psnr = 20*log10(255.0) - 10*log10(mse);
                mse = sqrt(mse);

//                Vector3f light = builder.imageset.lights[i];
//                float r = sqrt(light[0]*light[0] + light[1]*light[1]);
//                float elevation = asin(r);
                /*				cout << output << "," << types[builder.type] << "," << colorspaces[builder.colorspace] << ","
                    << builder.nplanes << ","<< builder.nmaterials << "," << builder.yccplanes[0] << ","
                    << size << "," << psnr << "," << mse << ","
                    << elevation << "," << builder.sigma << "," << builder.regularization << ","
                    << light[0] << "," << light[1] << endl; */
            }
            totmse /= builder.lights.size();
            double totpsnr = 20*log10(255.0) - 10*log10(totmse);
            totmse = sqrt(totmse);

            cout << output << "," << types[builder.type] << "," << colorspaces[builder.colorspace] << ","
				 << builder.nplanes << "," << builder.yccplanes[0] << ","
                 << size << "," << totpsnr << "," << totmse << endl;


            //cout << "PSNR: " << totpsnr << endl;
            return 0;
        }

        QDir out(output.c_str());
        ImageSet imgset;
		imgset.initFromFolder(input.c_str(), true);
        double mse = 0;
        mse = Rti::evaluateError(imgset, rti, out.filePath("error.png"), builder.skip_image);

        double psnr = 20*log10(255.0) - 10*log10(mse);
        mse = sqrt(mse);

        if(psnr == 0.0f) {
            cerr << "Failed reloading rti: " << builder.error << " !\n" << endl;
        }
        //type, colorspace, nplanes, nmaterials, ny

        Vector3f light = imgset.lights[builder.skip_image];
        float r = sqrt(light[0]*light[0] + light[1]*light[1]);
        float azimut = asin(r);
        cout << output << "," << types[builder.type] << "," << colorspaces[builder.colorspace] << ","
			 << builder.nplanes << "," << builder.yccplanes[0] << ","
             << size << "," << psnr << "," << mse << "," << azimut << "," << builder.sigma << "," << builder.regularization << "," << light[0] << "," << light[1] << endl;
    }

    return 0;
}



void initFromProject() {

}
