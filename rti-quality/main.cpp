#include "../src/getopt.h"
#include <iostream>
#include <QImage>
#include <QFileInfo>
#include <QDir>

#include "../src/jpeg_encoder.h"
#include "../src/imageset.h"
#include "../src/rti.h"

using namespace std;

/* Measure reconstruction error for PTM, HSH and CRB.
 */


int main(int argc, char *argv[]) {

	const char *usage = R"foo(Usage: rti-quality <img folder> <rti file> [OPTIONS]
Output basic info about the file and reconstruction error for RTI.

	<folder>: path to the source images and sphere.lp file
	<rti file>: .ptm .rti (hsh) format, .crb or crb web format folder

	-s: outputs csv columns: format, resolution, nmaterials, nplanes, size, error
	-e <type>: select type of error measure:
		mse: rgb mean square error (default)
	-p <nplanes>: number of planes used for reconstruction
	-i <img number>: test image to be saved, all to get all, csv to get a subset, :n to get one every n
	-p <prefix dir>: create directory to store images requested
	-E: skip error evaluation

)foo";

	QString request;
	QString prefix;
	vector<uint32_t> imgs;
	string imgs_path;
	string rti_path;
	string error_measure;
	bool skiperror = false;

	int c;
	while((c = getopt(argc, argv, "hse:i:Ep:")) != -1) {
		switch(c) {
		case 'e': error_measure = optarg; break;
		case 'i': request = QString(optarg); break;
		case 'p': prefix = QString(optarg); break;
		case 'E': skiperror = true; break;
		case 'h':
		case '?': cout << usage; break;
		default:
			cerr << "Unknown option: " << (char)c << "\n\n";
			cerr << usage;
			return -1;
		}
	}
	if(optind > argc - 2) {
		cerr << "expecting img folder and rti file\n\n";
		cerr << usage;
	}
	imgs_path = argv[optind++];
	rti_path = argv[optind++];

	Rti rti;
	if(!rti.load(rti_path.c_str())) {
		cerr << "Failed loading rti: " << rti_path << endl;
		return -1;
	}

	ImageSet imageset;
	if(!imageset.init(imgs_path.c_str()))
		return -1;

	if(request.isEmpty()) {
	} else if(request == "all") {
		for(int i = 0; i < imageset.lights.size(); i++)
			imgs.push_back(i);
	} else if(request.startsWith(":")) {
		
		request = request.right(1);
		int period = request.toInt();
		for(int i = 0; i < imageset.lights.size(); i += period)
			imgs.push_back(i);
	} else {
		imgs.push_back(request.toInt());
	}

	if(imgs.size()) {
		QImage img(rti.width, rti.height, QImage::Format_RGB888);
	
		if(!QDir().exists(prefix)) {
			if(!QDir().mkpath(prefix)) {
				cerr << "Failed to create directory: " << qPrintable(prefix) << endl;
				exit(-1);
			}
		}
		QDir dir(prefix);
		for(uint32_t im: imgs) {
			Vector3f light = imageset.lights[im];
			rti.render(light[0], light[1], img.bits());
			img.save(dir.filePath(QString("%1.png").arg(im)));
		}
		
	}
	if(!skiperror) {
		double mse = Rti::evaluateError(imageset, rti, (rti_path + "/error.png").c_str());
		double psnr = 20*log10(255.0) - 10*log10(mse);
		cout << "Psnr: " << psnr << endl;
	}

	return 0;
}
