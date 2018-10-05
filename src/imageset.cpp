#include <string>
#include <set>
#include <iostream>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include "imageset.h"
#include "jpeg_decoder.h"

using namespace std;

ImageSet::ImageSet(const char *path): width(0), height(0) {
	if(path)
		init(path);
}

ImageSet::~ImageSet() {
	//TODO decoders should take care to properly finish
	for(JpegDecoder *dec: decoders)
		delete dec;
}

bool ImageSet::init(const char *_path, bool ignore_filenames, int skip_image) {

	QDir dir(_path);
	QStringList lps = dir.entryList(QStringList() << "*.lp");
	if(lps.size() == 0) {
		cerr << "Could not find .lp file\n";
		return false;
	}
	QString sphere_path = dir.filePath(lps[0]);
	QFile sphere(sphere_path);
	if(!sphere.open(QFile::ReadOnly)) {
		cerr << "Could not open: " << qPrintable(sphere_path) << endl;
		return false;
	}

	QTextStream stream(&sphere);
	size_t n;
	stream >> n;


	QStringList img_ext;
	img_ext << "*.jpg" << "*.JPG";
	QStringList images = dir.entryList(img_ext);

	for(size_t i = 0; i < n; i++) {
		QString s;
		Vector3f light;
		stream >> s >> light[0] >> light[1] >> light[2];

		if(i == skip_image)
			continue;

		lights.push_back(light);
		QString filepath = dir.filePath(s);

		if(ignore_filenames) {
			if(images.size() != n) {
				cerr << "Lp number of lights (" << n << ") different from the number of images found (" << images.size() << ")\n";
				return false;
			}
			filepath = dir.filePath(images[i]);
		} else {
			//often path are absolute. TODO cleanup HERE!
			QFileInfo info(filepath);
			if(!info.exists()) {
				cerr << "Could not find image: " << qPrintable(s) << endl;
				return false;
			}
		}
		int w, h;
		JpegDecoder *dec = new JpegDecoder;
		if(!dec->init(filepath.toStdString().c_str(), w, h)) {
			cerr << "Failed decoding image: " << qPrintable(filepath) << endl;
			return false;
		}
		if(width && (width != (size_t)w || height != (size_t)h)) {
			cerr << "Inconsistent image size for " << qPrintable(filepath) << endl;
			return false;
		}
		width = (size_t)w;
		height = (size_t)h;
		decoders.push_back(dec);
	}
	return true;
}
void ImageSet::decode(size_t img, unsigned char *buffer) {
	decoders[img]->readRows(height, buffer);
}

void ImageSet::readLine(PixelArray &pixels) {
	pixels.resize(width, lights.size());
	std::vector<uint8_t> row(width*3);

	for(size_t i = 0; i < decoders.size(); i++) {
		decoders[i]->readRows(1, row.data());

		for(size_t x = 0; x < width; x++) {
			pixels(x, i).r = row[x*3 + 0];
			pixels(x, i).g = row[x*3 + 1];
			pixels(x, i).b = row[x*3 + 2];
		}
	}
}

//return a subset of k integers from 0 to n-1;
class StupidSampler {
public:
	set<uint32_t> res;
	StupidSampler() { srand(0); }
	set<uint32_t> &result(uint32_t k, uint32_t n) {
		res.clear();
		res.insert(0);
		while (res.size() < k)
			res.insert(rand()%n);
		return res;
	}
};

uint32_t ImageSet::sample(PixelArray &sample, uint32_t samplingrate) {
	uint32_t nsamples = width*height/samplingrate;
	if(nsamples > width*height)
		nsamples = width*height;

	uint32_t samplexrow = std::min(nsamples/height, width/4);
	nsamples = samplexrow*height;
	sample.resize(nsamples, lights.size());

	StupidSampler sampler;

	int offset = 0;
	vector<uint8_t> row(width*height*3);
	for(uint32_t y = 0; y < height; y++) {
		auto &selection = sampler.result(samplexrow, width);
		for(uint32_t i = 0; i < decoders.size(); i++) {
			JpegDecoder *dec = decoders[i];
			dec->readRows(1, row.data());
			int off = offset;
			for(int k: selection) {
				sample(off, i).r = row[k*3 + 0];
				sample(off, i).g = row[k*3 + 1];
				sample(off, i).b = row[k*3 + 2];

				off++;
			}
		}
		offset += samplexrow;
	}
	return nsamples;
}



void ImageSet::restart() {
	for(uint32_t i = 0; i < decoders.size(); i++)
		decoders[i]->restart();
}

