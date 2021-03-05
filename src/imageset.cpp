#include <string>
#include <set>
#include <iostream>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QImage>
#include "imageset.h"
#include "jpeg_decoder.h"

using namespace std;

ImageSet::ImageSet(const char *path) {
	if(path)
		init(path);
}

ImageSet::~ImageSet() {
	//TODO decoders should take care to properly finish
	for(JpegDecoder *dec: decoders)
		delete dec;
}

void ImageSet::parseLP(QString sphere_path, std::vector<Vector3f> &lights, std::vector<QString> &filenames, int skip_image) {

	QFile sphere(sphere_path);
	if(!sphere.open(QFile::ReadOnly))
		throw QString("Could not open: " + sphere_path);

	QTextStream stream(&sphere);
	bool ok;
	int n = stream.readLine().toInt(&ok);


	if(!ok || n <= 0 || n > 1000)
		throw QString("Invalid format or number of lights in .lp.");

	for(int i = 0; i < n; i++) {
		QString filename;
		Vector3f light;
		QString line = stream.readLine();
		QStringList tokens = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);
		if(tokens.size() != 4)
			throw QString("Invalid line in .lp: " + line);

		filename = tokens[0];
		for(int k = 0; k < 3; k++) {
			bool ok;
			light[k] = tokens[k+1].toDouble(&ok);
			if(!ok)
				throw QString("Failed reading light direction in: " + line);
		}
		double norm = light.norm();
		if(norm < 0.0001)
			throw QString("Light direction too close to the origin! " + line);

		light /= norm;

		if(i == skip_image)
			continue;

		lights.push_back(light);
		filenames.push_back(filename);
	}

}

bool ImageSet::init(const char *_path, bool ignore_filenames, int skip_image) {


	QDir dir(_path);
	QStringList lps = dir.entryList(QStringList() << "*.lp");
	if(lps.size() == 0) {
		cerr << "Could not find .lp file";
		return false;
	}
	QString sphere_path = dir.filePath(lps[0]);

	QStringList img_ext;
	img_ext << "*.jpg" << "*.JPG";
	images = dir.entryList(img_ext);


	try {
		std::vector<QString> filenames;
		parseLP(sphere_path, lights, filenames, skip_image);

		if(ignore_filenames) {
			if(images.size() != (int)filenames.size()) {
				QString error = QString("Lp number of lights (%1) different from the number of images found (%2)").arg(filenames.size(), images.size());
				throw error;
			}

		} else {
			throw "TODO: unimplemented.";

			//TODO check and remove absolute parth of the image;
			/*QString filepath = dir.filePath(images[i]);
			QFileInfo info(filepath);
			if(!info.exists()) {
				cerr << "Could not find image: " << qPrintable(s) << endl;
				return false;
			}*/
		}
	} catch(QString error) {
		cerr << qPrintable(error) << endl;
		return false;
	}
	return initImages(_path);
}

bool ImageSet::initImages(const char *_path) {
	QDir dir(_path);
	for(int i = 0; i < images.size(); i++) {
		QString filepath = dir.filePath(images[i]);
		int w, h;
		JpegDecoder *dec = new JpegDecoder;
		if(!dec->init(filepath.toStdString().c_str(), w, h)) {
			cerr << "Failed decoding image: " << qPrintable(filepath) << endl;
			return false;
		}
		if(width && (width != w || height != h)) {
			cerr << "Inconsistent image size for " << qPrintable(filepath) << endl;
			return false;
		}
		right = image_width = width = w;
		bottom = image_height = height = h;

		decoders.push_back(dec);
	}
	return true;
}
void ImageSet::crop(int _left, int _top, int _width, int _height) {
	left = _left;
	top = _top;
	if(_width > 0) {
		width = _width;
		height = _height;
	}
	right = left + width;
	bottom = top + height;
	if(left < 0 || left >= right || top < 0 || top >= bottom || right > image_width || bottom > image_height) {
		cout << "Invalid crop parameters: " << endl;
		cout << "left: " << left << " top: " << top << " right: " << right << " bottom: " << bottom << " width: " << width << " height: " << height << endl;
		throw "Invalid crop parameters";
	}
}

QImage ImageSet::maxImage(std::function<bool(std::string stage, int percent)> *callback) {
	int w = image_width;
	int h = image_height;
	QImage image(w, h, QImage::Format::Format_RGB888);
	image.fill(0);
	
	uint8_t *row = new uint8_t[w*h*3];
	
	restart();
	for(int y = 0; y < image_height; y++) {
		if(callback) {
			bool keep_going = (*callback)(std::string("Sampling images"), 100*(y-top)/height);
			if(!keep_going)
				throw 1;
		}
		uint8_t *rowmax = image.scanLine(y);
		for(uint32_t i = 0; i < decoders.size(); i++) {
			JpegDecoder *dec = decoders[i];
			dec->readRows(1, row);
			
			for(int x = 0; x < image_width; x++) {
				rowmax[x*3 + 0] = std::max(rowmax[x*3 + 0], row[x*3 + 0]);
				rowmax[x*3 + 1] = std::max(rowmax[x*3 + 1], row[x*3 + 0]);
				rowmax[x*3 + 2] = std::max(rowmax[x*3 + 2], row[x*3 + 0]);
			}
		}
	}
	delete []row;
	return image;
}

void ImageSet::decode(size_t img, unsigned char *buffer) {
	throw "TO FIX for crop!";
	decoders[img]->readRows(height, buffer);
}

void ImageSet::readLine(PixelArray &pixels) {
	if(current_line == 0)
		skipToTop();
	pixels.resize(width, lights.size());
	//TODO: no need to allocate EVERY time.
	std::vector<uint8_t> row(image_width*3);

	for(size_t i = 0; i < decoders.size(); i++) {
		decoders[i]->readRows(1, row.data());
		for(int x = left; x < right; x++) {
			pixels(x - left, i).r = row[x*3 + 0];
			pixels(x - left, i).g = row[x*3 + 1];
			pixels(x - left, i).b = row[x*3 + 2];
		}
	}
	current_line++;
}

//return a subset of k integers from 0 to n-1;
class StupidSampler {
public:
	set<uint32_t> res;
	StupidSampler() { srand(0); }
	set<uint32_t> &result(uint32_t k, uint32_t n) {
		res.clear();
		//res.insert(0);
		while (res.size() < k)
			res.insert(rand()%n);
		return res;
	}
};

uint32_t ImageSet::sample(PixelArray &resample, uint32_t ndimensions, function<void(Color3f *sample, Color3f *resample)> resampler, uint32_t samplingram) {
	if(current_line == 0)
		skipToTop();

	uint32_t bytes_per_sample = ndimensions*12;
	uint32_t nsamples = samplingram*((1<<20)/bytes_per_sample);
	
	if(nsamples > (uint32_t)width*height)
		nsamples = width*height;

	uint32_t samplexrow = std::min((int)(nsamples/height), (int)(width/4));
	nsamples = samplexrow*height;
	resample.resize(nsamples, ndimensions);

	StupidSampler sampler;
	PixelArray sample(samplexrow, lights.size());

	uint32_t offset = 0;
	vector<uint8_t> row(image_width*3);
	for(int y = top; y < bottom; y++) {
		if(callback) {
			bool keep_going = (*callback)(std::string("Sampling images"), 100*(y-top)/height);
			if(!keep_going)
				throw 1;
		}

		auto &selection = sampler.result(samplexrow, width);
		for(uint32_t i = 0; i < decoders.size(); i++) {
			JpegDecoder *dec = decoders[i];
			dec->readRows(1, row.data());
			uint32_t x = 0;
			for(int k: selection) {
				Color3f &pixel = sample(x, i);
				pixel.r = row[(k+left)*3 + 0];
				pixel.g = row[(k+left)*3 + 1];
				pixel.b = row[(k+left)*3 + 2];

				x++;
			}
		}
		for(uint32_t x = 0; x < samplexrow; x++)
			resampler(sample(x), resample(offset + x));

		offset += samplexrow;
	}
	return nsamples;
}

void ImageSet::restart() {
	for(uint32_t i = 0; i < decoders.size(); i++)
		decoders[i]->restart();
	
	current_line = 0;
}

void ImageSet::skipToTop() {
	std::vector<uint8_t> row(image_width*3);

	for(uint32_t i = 0; i < decoders.size(); i++) {
		for(int y = 0; y < top; y++)
			decoders[i]->readRows(1, row.data());
		
		if(callback) {
			bool keep_going = (*callback)(std::string("Skipping cropped part"), 100*i/decoders.size());
			if(!keep_going)
				throw 1;
		}
	}
	current_line += top;
}

