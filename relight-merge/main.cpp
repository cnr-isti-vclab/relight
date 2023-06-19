#include <QStringList>
#include <QDir>
#include <vector>
#include <iostream>
#include "../src/getopt.h"

#include "../src/rti.h"
#include "../relight-cli/rtibuilder.h"
#include "../src/jpeg_encoder.h"

using namespace std;

int main(int argc, char *argv[]) {


	bool use_rti_quality = true;
	uint32_t quality = 95;
	std::vector<float> scale;
	std::vector<float> bias;
	std::vector<float> min;
	std::vector<float> max;

	opterr = 0;
	char c;
	while ((c  = getopt (argc, argv, "q:")) != -1) {
		switch (c)
		{
		case 'q':
			quality = atoi(optarg);
			if(quality <= 0 || quality > 100) {
				cerr << "Invalid quality (-q) value: " << quality << ", it should be between 1 and 100\n";
				exit(-1);
			}
			use_rti_quality = false;
			break;
		default:
			cerr << "Unknown error!\n" << endl;
			return -1;
		}
	}
	if(argc - optind < 3) {
		cerr << "Usage: " << argv[0] << " [OPTIONS] <relight folder 1> ... <relight folder 2> <output>\n\n";
		cerr << "This code uniforms scale and bias for the coefficient planes so that they can be merged.\n"
				"The religth folders must have the same basis, number of planes, etc. \n\n";
		cerr << "Options:\n";
		cerr << "	-q <quality>: jpeg quality [0-100]\n";
		return 0;
	}

	//load all RTI json (not the planes)
	vector<RtiBuilder> rtis(argc - optind - 1);
	for(size_t i = 0; i < rtis.size(); i++) {
		RtiBuilder &rti = rtis[i];
		char *path = argv[optind + i];
		bool success = rti.load(path, false);
		if(!success) {
			cerr << "Could not load rti: " << path << endl;
			return -1;
		}
		//check all RTI have the same parameters
		if(i == 0) {
			min.resize(rti.nplanes, 1e20f);
			max.resize(rti.nplanes, -1e20f);
			continue;
		}
		if(rti.basis != rtis[0].basis) {
			cerr << "Rti basis for " << path << " is different." << endl;
			return -1;
		}
		if(rti.colorspace != rtis[0].colorspace) {
			cerr << "Rti colorspace for " << path << " is different." << endl;
			return -1;
		}
		if(rti.nplanes != rtis[0].nplanes) {
			cerr << "Rti number of planes for " << path << " is different." << endl;
			return -1;
		}

		for(size_t k = 0; k < rti.nplanes; k++) {
			float s = rti.material.planes[k].scale;
			float b = rti.material.planes[k].bias;
			min[k] = std::min((0 - b)*s, min[k]);
			max[k] = std::max((1 - b)*s, max[k]);
		}
	}
	for(int i = 0; i < min.size(); i++) {
		scale.push_back(max[i] - min[i]);
		bias.push_back(-min[i] / (max[i] - min[i]));
	}

	//create output directory
	QString output = argv[argc-1];

	QDir output_dir(output);
	if(!output_dir.exists()) {
		QDir here("./");
		if(!here.mkdir(output)) {
			cerr << "Could not create output directory.\n";
			return -1;
		}
	}

	for(size_t i = 0; i < rtis.size(); i++) {
		char *path = argv[optind + i];
		QDir input_rti_dir(path);

		if(!output_dir.mkdir(input_rti_dir.dirName())) {
			cerr << "Could not create remapped RTI directory: " << qPrintable(input_rti_dir.dirName()) << endl;
			return -1;
		}

		RtiBuilder &rti = rtis[i];
		rti.loadData(path);
		for(int i = 0; i < rti.nplanes; i++) {
			auto &plane = rti.planes[i];
			for(uint8_t &c: plane) {
				float v = (c/255.0f - rti.material.planes[i].bias)*rti.material.planes[i].scale;
				c = std::max(0, std::min(255, int(255*(v/scale[i] + bias[i]))));
			}
		}

		rti.scale = scale;
		rti.bias = bias;
		//save json
		QDir output_rti_dir(output_dir.filePath(input_rti_dir.dirName()));
		uint32_t rti_quality = use_rti_quality ? rti.quality : quality;
		rti.saveJSON(output_rti_dir, rti_quality);
		int width = rti.width;
		int height = rti.height;
		vector<uint8_t> line(width*3);

		for(int i = 0; i < rti.nplanes/3; i++) {
			QString filename = output_rti_dir.filePath(QString("plane_%1.jpg").arg(i));
			JpegEncoder enc;
			enc.setQuality(rti_quality);
			enc.setColorSpace(JCS_RGB, 3);
			enc.setJpegColorSpace(JCS_RGB);
			enc.init(filename.toStdString().c_str(), width, height);


			for(int y = 0; y < height; y++) {
				for(int32_t x = 0; x < width; x++) {
					int32_t p = y*width + x;
					for(int k = 0; k < 3; k++)
						line[x*3 + k] = rti.planes[i*3 + k][p];
				}
				enc.writeRows(line.data(), 1);
			}
			enc.finish();
		}
		rti.planes = std::vector<std::vector<uint8_t>>();
	}


	return 0;
}
