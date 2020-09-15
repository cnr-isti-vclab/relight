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
	images = dir.entryList(img_ext);

	for(size_t i = 0; i < n; i++) {
		QString s;
		Vector3f light;
		stream >> s >> light[0] >> light[1] >> light[2];

		if(i == skip_image)
			continue;

		lights.push_back(light);

		if(ignore_filenames) {
			if(images.size() != n) {
				cerr << "Lp number of lights (" << n << ") different from the number of images found (" << images.size() << ")\n";
				return false;
			}

		} else {
			throw "TODO: remove absolute parth of the image.";
			//often path are absolute. TODO cleanup HERE!
			QString filepath = dir.filePath(images[i]);
			QFileInfo info(filepath);
			if(!info.exists()) {
				cerr << "Could not find image: " << qPrintable(s) << endl;
				return false;
			}
		}
	}
	return initImages(_path);
}

bool ImageSet::initImages(const char *_path) {
	QDir dir(_path);
	for(size_t i = 0; i < images.size(); i++) {
		QString filepath = dir.filePath(images[i]);
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
		right = image_width = width = (size_t)w;
		bottom = image_height = height = (size_t)h;

		decoders.push_back(dec);
	}
	return true;
}
void ImageSet::crop(size_t _left, size_t _top, size_t _right, size_t _bottom) {
	left = _left;
	top = _top;
	right = _right > 0 ? _right : width;
	bottom = _bottom > 0 ? _bottom : height;
	if(left < 0 || left >= right || top < 0 || top >= bottom || right > image_width || bottom > image_height)
		throw "Invalid crop parameters";
	width = right - left;
	height = bottom - top;

	skipToTop();
}

void ImageSet::decode(size_t img, unsigned char *buffer) {
	throw "TO FIX!";
	decoders[img]->readRows(height, buffer);
}

void ImageSet::readLine(PixelArray &pixels) {
	pixels.resize(width, lights.size());
	std::vector<uint8_t> row(image_width*3);

	for(size_t i = 0; i < decoders.size(); i++) {
		decoders[i]->readRows(1, row.data());
		for(size_t x = left; x < right; x++) {
			pixels(x - left, i).r = row[x*3 + 0];
			pixels(x - left, i).g = row[x*3 + 1];
			pixels(x - left, i).b = row[x*3 + 2];
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

uint32_t ImageSet::sample(PixelArray &sample, uint32_t samplingrate, std::function<void(std::string stage, int percent)> *callback) {
	uint32_t nsamples = width*height/samplingrate;
	if(nsamples > width*height)
		nsamples = width*height;

	uint32_t samplexrow = std::min(nsamples/height, width/4);
	nsamples = samplexrow*height;
	sample.resize(nsamples, lights.size());

	StupidSampler sampler;

	int offset = 0;
	vector<uint8_t> row(image_width*3);
	for(uint32_t y = top; y < bottom; y++) {
		if(callback)
			(*callback)(std::string("Sampling images"), 100*(y-top)/height);

		auto &selection = sampler.result(samplexrow, width);
		for(uint32_t i = 0; i < decoders.size(); i++) {
			JpegDecoder *dec = decoders[i];
			dec->readRows(1, row.data());
			int off = offset;
			for(int k: selection) {
				k += left;
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
	cout << "Restarting\n" << endl;

	for(uint32_t i = 0; i < decoders.size(); i++)
		decoders[i]->restart();

	skipToTop();
	cout << "Restarted\n" << endl;
}

void ImageSet::skipToTop() {
	cout << "Skipping\n" << endl;
	std::vector<uint8_t> row(image_width*3);

	for(uint32_t i = 0; i < decoders.size(); i++) {
		for(size_t x = 0; x < top; x++)
			decoders[i]->readRows(1, row.data());
	}
	cout << "Skipped\n" << endl;
}

