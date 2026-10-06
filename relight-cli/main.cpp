#include "../src/cli/rtibuilder.h"
#include "../src/normals/normalstask.h"
#include "../src/brdf/brdftask.h"
#include "../src/lp.h"
#include "../src/sphere.h"
#include "../src/spherelocator.h"
#include "../src/image.h"
#include "../src/exif.h"
#include "../src/image_decoder.h"

#include "../src/getopt.h"
extern int opterr;

#include <QDir>
#include <QImage>
#include <QImageReader>
#include <QFile>
#include <QTextStream>
#include <QElapsedTimer>

#include <Eigen/Core>
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
	cout << "Usage: relight-cli [-bpqy3PnmMwkrsSRQcCeEvAF]<input folder> [output folder]\n\n";
	cout << "       relight-cli [-q] <input.ptm|.rti> [output folder]\n\n";
	cout << "       relight-cli [-q] <input.json> [output.ptm]\n\n";
	cout << "\tinput folder containing a .lp or .dome with number of photos and light directions\n";
	cout << "\toptional output folder (default ./)\n\n";
	cout << "\t  -b <basis>: rbf(default), ptm, lptm, hsh, yrbf, bilinear, skip\n";
	cout << "\t  -p <int>  : number of planes (default: 9)\n";
	cout << "\t  -q <int>  : jpeg quality (default: 95)\n";
	cout << "\t  -y <int>  : number of Y planes in YCC\n\n";
	cout << "\t  -3 <radius[:offset]>: 3d light positions processing, ratio radiud dome/image width\n               and optionally vertical offset of the center of the sphere to the surface.\n";

	cout << "\t  -P <pixel size in MM>: this number is saved in .json output and within image metadata\n";
	cout << "\t  -n        : extract normals\n";
	cout << "\t  -m        : extract mean image\n";
	cout << "\t  -M        : extract median image (7/8th quantile) \n";

	cout << "\t  -w        : number of workers (default 8)\n";
	cout << "\t  -k <int>x<int>+<int>+<int>: Cropping extracts only the widthxheight+offx+offy part\n";
	cout << "\t  -A        : experimental: locate the reflective spheres and compute the light directions,\n";
	cout << "\t              no .lp needed (jpg, png, tif). The directions are saved in lights.lp in the output folder.\n";
	cout << "\t  -F <float>: with -A, 35mm equivalent focal length (default: from EXIF)\n";

	cout << "\nIgnore exotic parameters below here\n\n";
	cout << "\n  -H        : fix overexposure in ptm and hsh due to bad sampling\n";
	cout << "\t  -r <int>  : side of the basis function (default 8, 0 means rbf interpolation)\n";
	cout << "\t  -s <int>  : sampling RAM for pca  in MB (default 500MB)\n";
	cout << "\t  -S <float>: sigma in rgf gaussian interpolation default 0.125 (~100 img)\n";
	cout << "\t  -R <float>: regularization coeff for bilinear default 0.1\n";
	cout << "\t  -Q <float>: quantile for histogram-based range compression (default 0.995 = 99.5%)\n";
	cout << "\t              Clamps outliers to improve quantization resolution for most pixels\n";
	cout << "\t  -c <float>: coeff quantization (to test!) default 1.5\n";
	cout << "\t  -C        : apply chroma subsampling \n";
	cout << "\t  -I <preserve|srgb|displayp3>: ICC color profile handling (default: preserve)\n";
	cout << "\t  -e        : evaluate reconstruction error (default: false)\n";
	cout << "\t  -E <int>  : evaluate error on a single image (but remove it for fitting)\n";

	cout << "\n\nTesting options, will use the input folder as an RTI source: \n";

	cout << "\t  -D <path> : directory to store rebuilt images\n";
	cout << "\t  -L <x:y:z> : reconstruct only one image from light parameters, output is the filename\n";
	cout << "\t  -v : verbose, prints progress info\n";
}

//convert PTM into relight format
int convertRTI(const char *file, const char *output, int quality);

//converts relight into PTM format
int convertToRTI(const char *file, const char *output, int quality, QString &msg);

void test(std::string input, std::string output,  Eigen::Vector3f light, float test_sigma = 0.0f) {

	Rti rti;
	if(!rti.load(input.c_str())) {
		cerr << "Failed loading rti: " << input << " !\n" << endl;
		return;
	}
	if(test_sigma != 0.0f)
		rti.sigma = test_sigma;

	light = light / light.norm();
	//   rti.render(light[0], light[1], buffer.data());
	QImage img(rti.width, rti.height, QImage::Format_RGBA8888);
	rti.render(light[0], light[1], img.bits(), 4);
	img.save(output.c_str());
}

//size: downscale, clip: part of the image at full resolution.
static QImage readImage(const QString &filename, QSize size = QSize(), QRect clip = QRect()) {
	QImageReader reader(filename);
	reader.setAutoTransform(false);
	if(size.isValid())
		reader.setScaledSize(size);
	if(clip.isValid())
		reader.setClipRect(clip);
	QImage img = reader.read();
	if(!img.isNull())
		return img;

	//formats Qt can't read.
	ImageDecoder dec;
	int w = 0, h = 0;
	if(!dec.init(filename.toStdString().c_str(), w, h))
		return QImage();
	int ch = dec.numChannels();
	size_t row_bytes = dec.rowSize();
	std::vector<uint8_t> buffer(size_t(h)*row_bytes);
	dec.readRows(h, buffer.data());
	//no finish(): the destructor releases the decoder.
	img = QImage(w, h, ch == 4 ? QImage::Format_RGBA8888 : ch == 1 ? QImage::Format_Grayscale8 : QImage::Format_RGB888);
	for(int y = 0; y < h; y++)
		memcpy(img.scanLine(y), buffer.data() + size_t(y)*row_bytes, row_bytes);
	if(clip.isValid())
		img = img.copy(clip);
	if(size.isValid())
		img = img.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	return img;
}

//experimental: locate the reflective spheres, find the highlights and compute the light directions.
//Images without highlights are removed from the list. focal: 35mm equivalent, 0 to read it from EXIF.
static void lightsFromSpheres(const QString &folder, QStringList &images, Dome &dome, float focal, bool verbose, std::function<bool(QString, int)> *callback) {
	QDir dir(folder);
	images = dir.entryList(QStringList() << "*.jpg" << "*.JPG" << "*.jpeg" << "*.png" << "*.PNG" << "*.tif" << "*.TIF" << "*.tiff", QDir::Files, QDir::Name);
	int n = images.size();
	if(n < 3)
		throw QString("Not enough images in folder: ") + folder;

	QImage first = readImage(dir.filePath(images[0]));
	if(first.isNull())
		throw QString("Failed loading image: ") + images[0];
	Lens lens;
	lens.width = first.width();
	lens.height = first.height();
	try {
		Exif exif;
		exif.parse(dir.filePath(images[0]));
		lens.readExif(exif);
	} catch(QString) {}
	if(!lens.pixelSizeX)
		lens.pixelSizeX = lens.pixelSizeY = 36.0/lens.width;
	if(focal)
		lens.focalLength = focal;
	if(!lens.focalLength) {
		cerr << "No focal length in EXIF, assuming a distant camera. Use -F to set it." << endl;
		lens.focalLength = 1000;
	}

	SphereLocator locator(first.size());
	auto load = [&](int i, QSize size, QRect clip) { return readImage(dir.filePath(images[i]), size, clip); };
	vector<SphereLocator::Circle> circles = locator.run(n, load, callback);
	if(circles.empty())
		throw QString("No reflective sphere found.");

	vector<Sphere *> spheres;
	for(SphereLocator::Circle &circle: circles) {
		Sphere *sphere = new Sphere(n);
		sphere->border = circle.points();
		sphere->fit();
		spheres.push_back(sphere);
		if(verbose)
			cout << "\nSphere center: " << sphere->center.x() << " " << sphere->center.y() << " radius: " << sphere->radius << endl;
	}
	for(int i = 0; i < n; i++) {
		QImage img = readImage(dir.filePath(images[i]));
		if(img.size() != first.size())
			throw QString("Image has a different size: ") + images[i];
		for(Sphere *sphere: spheres)
			sphere->findHighlight(img, i, false);
		if(callback && !(*callback)("Detecting highlights", 100*(i + 1)/n))
			throw QString("Cancelled.");
	}
	vector<Image> set;
	for(QString &image: images)
		set.push_back(Image(image));
	dome.fromSpheres(set, spheres, lens);
	for(Sphere *sphere: spheres)
		delete sphere;

	QStringList found;
	vector<Eigen::Vector3f> directions;
	for(int i = 0; i < n; i++) {
		if(dome.directions[i].isZero()) {
			cerr << "\nNo highlight found, skipping: " << qPrintable(images[i]) << endl;
			continue;
		}
		found.push_back(images[i]);
		directions.push_back(dome.directions[i].normalized());
	}
	images = found;
	dome.directions = directions;
	dome.positions3d.clear();
	dome.positionsSphere.clear();
	dome.lightConfiguration = Dome::DIRECTIONAL;
}

static void saveLP(const QString &filename, const QStringList &images, const vector<Eigen::Vector3f> &directions) {
	QFile file(filename);
	if(!file.open(QFile::WriteOnly))
		throw QString("Failed saving: ") + filename;
	QTextStream stream(&file);
	stream << images.size() << "\n";
	for(int i = 0; i < images.size(); i++)
		stream << images[i] << " " << directions[i][0] << " " << directions[i][1] << " " << directions[i][2] << "\n";
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
	bool skip_rti = false;
	float sigma_test = 0.0f;
	Dome dome;
	int quality = 95;
	bool evaluate_error = false;
	QString redrawdir;
	bool relighted = false;
	Eigen::Vector3f light;
	bool verbose = false;
	bool auto_spheres = false;
	float auto_focal = 0.0f;

	opterr = 0;
	char c;
	while ((c  = getopt (argc, argv, "hmMn3:r:d:q:p:s:c:reE:b:y:S:R:CD:Q:L:k:P:I:vAF:")) != -1)
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
			builder.resolution = res;
			if(res < 0 || res == 1 || res == 2 || res > 20) {
				cerr << "Invalid resolution (must be 0 or >= 2 && <= 20)!\n" << endl;
				return 1;
			}
			break;
		}
		case 'P':
			builder.imageset.pixel_size = atof(optarg);
			if(builder.imageset.pixel_size <= 0) {
				cerr << "Invalid parameter pixelSize (-P): " << optarg << endl;
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

			} else if(b == "skip") {
				skip_rti = true;

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
			dome.lightConfiguration = Dome::SPHERICAL;
			dome.imageWidth = 1.0f;
			dome.domeDiameter = 2.0f*float(atof(optarg));

			QString params(optarg);
			if(params.contains(':')) {
				dome.verticalOffset = params.split(':')[1].toDouble();
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
			if(sigma > 0) {
				sigma_test = builder.sigma = sigma;
			}
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
		case 'Q': {
			float quantile = float(atof(optarg));
			if(quantile > 0.0f && quantile < 1.0f)
				builder.rangeQuantile = quantile;
			else {
				cerr << "Range quantile must be between 0 and 1 (e.g., 0.995 for 99.5%)!\n" << endl;
				return 1;
			}
			break;
		}
		case 'C':
			builder.chromasubsampling = true;
			break;
		case 'I': {
			string mode = optarg;
			if(mode == "linear") {
				builder.colorProfileMode = COLOR_PROFILE_LINEAR_RGB;
			} else if(mode == "srgb") {
				builder.colorProfileMode = COLOR_PROFILE_SRGB;
			} else if(mode == "displayp3") {
				builder.colorProfileMode = COLOR_PROFILE_DISPLAY_P3;
			} else {
				cerr << "Invalid color profile mode: " << optarg << " (use 'linear', 'srgb', or 'displayp3')\n" << endl;
				return 1;
			}
			break;
		}

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
		case 'A':
			auto_spheres = true;
			break;
		case 'F':
			auto_focal = atof(optarg);
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

	if(dome.lightConfiguration != Dome::DIRECTIONAL && builder.type == RtiBuilder::RBF) {
		cerr << "RBF basis do not support positional lights (for the moment)\n";
		return 1;
	}

	/* Sanity checks, TODO: move into RTI builder! */

	switch(builder.type) {
	case Rti::PTM:
		if(builder.colorspace == Rti::LRGB && builder.nplanes != 9) {
			cerr << "lptm basis requires 9 coefficient planes (option -p 9)\n";
			return -1;
		}
		if(builder.colorspace == Rti::RGB && builder.nplanes != 18) {
			cerr << "ptm basis requires 18 coefficient planes (option -p 18)\n";
			return -1;
		}
		break;
	case Rti::HSH:
		if(builder.colorspace == Rti::RGB && (builder.nplanes != 27 && builder.nplanes != 12)) {
			cerr << "hsh basis requires 12 or 27 coefficient planes (option -p 12 or -p 27)\n";
			return -1;
		}
		break;
	default: break;
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
	const char *output = "./";
	if(optind < argc)
		output = argv[optind++];

	if(relighted) {
		if(redrawdir.isNull()) {
			cerr << "Specify an output image filename using -D option\n" << endl;
			return -1;
		}
		test(input, redrawdir.toStdString(), light, sigma_test);
		return 0;
	}

	if(skip_rti && auto_spheres) {
		cerr << "Option -A is not supported with -b skip (yet)\n" << endl;
		return 1;
	}
	if(skip_rti) {

		try {
			QFileInfo info(output);
			QString folder;
			QString normals_filename;
			QString albedo_filename;
			QString ext = ".jpg";
			if(info.isDir()) {
				folder = input.c_str();
				albedo_filename = "normals.jpg";
				albedo_filename = "albedo.jpg";
			} else {
				folder = info.absolutePath();
				normals_filename = info.fileName();
				albedo_filename = info.fileName();
			}
			QDir dir(input.c_str());

			//TODO: refactor this!
			QStringList lp_ext;
			lp_ext << "*.lp";
			QStringList lps = dir.entryList(lp_ext);
			if(lps.size() == 0)
				throw QString("Could not find a .lp file in the folder");

			dome.parseLP(dir.filePath(lps[0]));

			Crop crop;
			if(builder.crop[2] != 0) { //no crop specitied
				crop.setRect(QRect(builder.crop[0], builder.crop[1], builder.crop[2], builder.crop[3]));
			}

			if(builder.savemeans || builder.savemedians) {
				//image_set.saveMean(output.c_str(), builder.quality);
				BrdfTask brdf;
				brdf.initFromFolder(input.c_str(), dome, crop);
				brdf.parameters.albedo = builder.savemeans ? BrdfParameters::MEAN : BrdfParameters::MEDIAN;
				brdf.parameters.path = folder;
				brdf.parameters.albedo_path = albedo_filename;
				brdf.run();
			}

			if(builder.savenormals) {
				NormalsTask normals;

				normals.initFromFolder(input.c_str(), dome, crop);
				normals.parameters.path = folder;
				normals.parameters.normalsname = normals_filename.mid(0, -4);
				normals.run();
			}

		} catch(QString error) {
			cerr << qPrintable(error) << endl;
			return 1;
		}
		return 0;
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
	if(!info.exists()) {
		cerr << "Input \"" << input << "\" doesn't seems to exist." << endl;
		return 1;
	}
	if(info.isFile()) {
		if(info.suffix() == "relight") {
			if(!builder.setupFromProject(input)) {
				cerr << builder.error << "\n" << endl;
				return 1;
			}

		} else if(info.suffix() == "json") {
			try {
				QString msg;
				int status = convertToRTI(input.c_str(), output, quality, msg);
				if(status == 1) {
					cerr << qPrintable(msg) << endl;
				}
			} catch(QString error) {
				cerr << qPrintable(error) << endl;
				return 1;
			}
			return 0;

		} else if(info.suffix() == "rti" || info.suffix() == "ptm") {
			try {
				convertRTI(input.c_str(), output, quality);

			} catch(QString error) {
				cerr << qPrintable(error) << endl;
				return 1;
			}

		} else {
			cerr << "Input parameter (" << input << ") is an unknown type, relight-cli can process .relight, .json, .rti or .ptm files" << endl;
			return 1;
		}
	} else if(info.isDir()) {

		QStringList auto_images; //found with -A, otherwise all .jpg
		if(auto_spheres) {
			try {
				lightsFromSpheres(input.c_str(), auto_images, dome, auto_focal, verbose, callback);
				QString out(output);
				QDir out_dir = (out.endsWith(".ptm") || out.endsWith(".rti")) ? QFileInfo(out).absoluteDir() : QDir(out);
				out_dir.mkpath(".");
				saveLP(out_dir.filePath("lights.lp"), auto_images, dome.directions);
			} catch(QString error) {
				cerr << qPrintable(error) << endl;
				return 1;
			}
		} else {
			//look for .lp
			QDir dir(input.c_str());
			QStringList lp_ext;
			lp_ext << "*.lp";
			QStringList lps = dir.entryList(lp_ext);
			if(lps.size() == 0)
				throw QString("Could not find a .lp file in the folder");

			dome.parseLP(dir.filePath(lps[0]));
		}

		if(!builder.setupFromFolder(input, dome, auto_images)) {
			cerr << builder.error << " !\n" << endl;
			return 1;
		}
	} else {
		cerr << "Input\"" << input << "\" is not a file or a directory." << endl;
		return 1;
	}

	QString out = output;
	int size = 0; //size of the output

	try {
		if(out.endsWith(".ptm") || out.endsWith(".rti"))
			builder.commonMinMax = true; //needed by legacy formats

		builder.init(callback);

		if(out.endsWith(".ptm")) {
			size = builder.savePTM(output);
		} else if(out.endsWith(".rti")) {

			size = builder.saveUniversal(output);
		} else {
			size = builder.save(output, quality);
		}
		if(size == 0) {
			cerr << "Failed saving: " << builder.error << " !\n" << endl;
			return 1;
		}
	} catch(QString error) {
		cerr << qPrintable(error) << endl;
		return 1;
	}

	int time = timer.restart();
	if(time < 10000)
		cout << "\nDone in: " << time << "ms" << endl;
	else
		cout << "\nDone in: " << time/1000 << "s" << endl;

	if(redrawdir.size()) {
		Rti rti;
		if(!rti.load(output)) {
			cerr << "Failed loading rti: " << output << " !\n" << endl;
			return 1;
		}

		QDir dir(redrawdir);
		if(!dir.exists()) {
			cerr << "Directory for redraw not found!\n" << endl;
			return 1;
		}
		for(size_t i = 0; i < builder.lights.size(); i++) {
			Eigen::Vector3f &rlight = builder.lights[i];
			//  rti.render(rlight[0], rlight[1], buffer.data());

			QImage img(rti.width, rti.height, QImage::Format_RGBA8888);
			rti.render(rlight[0], rlight[1], img.bits(), 4);
			img.save(dir.filePath( builder.imageset.images[i]));
		}
	}

	if(evaluate_error) {
		Rti rti;
		if(!rti.load(output)) {
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

		QDir out(output);
		builder.setupFromFolder(input.c_str(), dome);
		ImageSet &imgset = builder.imageset;

		double mse = 0;
		mse = Rti::evaluateError(imgset, rti, out.filePath("error.png"), builder.skip_image);

		double psnr = 20*log10(255.0) - 10*log10(mse);
		mse = sqrt(mse);

		if(psnr == 0.0f) {
			cerr << "Failed reloading rti: " << builder.error << " !\n" << endl;
		}
		//type, colorspace, nplanes, nmaterials, ny

		Eigen::Vector3f light = dome.directions[builder.skip_image];
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
