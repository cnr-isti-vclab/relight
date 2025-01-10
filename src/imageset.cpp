#include "imageset.h"
#include "dome.h"
#include "jpeg_decoder.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <string>
#include <set>
#include <iostream>


#include <assert.h>
using namespace std;
using namespace Eigen;

ImageSet::ImageSet(const char *path) {
	if(path)
		initFromFolder(path);
}

ImageSet::~ImageSet() {
	//TODO decoders should take care to properly finish
	for(JpegDecoder *dec: decoders)
		delete dec;
}



bool ImageSet::initFromFolder(const char *_path, int skip_image) {

	QDir dir(_path);

	QStringList img_ext;
	img_ext << "*.jpg" << "*.JPG";
	images = dir.entryList(img_ext);

	if(skip_image >= 0)
		images.removeAt(skip_image);

	return initImages(_path);
}

bool ImageSet::initFromProject(QJsonObject &obj, const QString &filename) {

	QFileInfo info(filename);
	QDir folder = info.dir();
	path = QString(folder.absolutePath());
	folder.cd(obj["folder"].toString());

	for(auto img: obj["images"].toArray()) {
		auto image = img.toObject();

		bool skip = image["skip"].toBool();
		if(skip)
			continue;

		QString filename = image["filename"].toString();

		QFileInfo imginfo(folder.filePath(filename));
		if(!imginfo.exists())
			throw QString("Could not find the image: " + filename) + " in folder: " + folder.absolutePath();

		images.push_back(filename);
	}

	bool ok = initImages(folder.path().toStdString().c_str());
	if(!ok)
		return false;

	//needs image_width and height to apply crop
	int range[4] = { 0, 0, 0, 0 };
	if(obj.contains("crop")) {
		QJsonObject c = obj["crop"].toObject();
		range[0] = c["left"].toInt();
		range[1] = c["top"].toInt();
		range[2] = c["width"].toInt();
		range[3] = c["height"].toInt();
		crop(range[0], range[1], range[2], range[3]);
	}
	return true;
}

void ImageSet::initFromDome(Dome &dome) {
	light3d = dome.lightConfiguration != Dome::DIRECTIONAL;

	assert(image_width != 0);
	pixel_size = dome.imageWidth / image_width;
	switch(dome.lightConfiguration) {
		case Dome::DIRECTIONAL:
			setLights(dome.directions, dome.lightConfiguration);
		break;
		case Dome::SPHERICAL:			
			dome.updateSphereDirections();
			setLights(dome.positionsSphere, dome.lightConfiguration);
		break;
		case Dome::LIGHTS3D:
			setLights(dome.positions3d, dome.lightConfiguration);
		break;
	}
}

void ImageSet::setLights(std::vector<Eigen::Vector3f> &lights, Dome::LightConfiguration configuration) {
	lights1 = lights;
	light3d = (configuration != Dome::DIRECTIONAL);
	if(light3d) {
		idealLightDistance2 = 0.0f;
		for(auto l: lights1) {
			idealLightDistance2 += l.squaredNorm();
		}
		idealLightDistance2 /= lights1.size();
	}
	if(lights1.size() != size_t(images.size()))
		throw QString("Number of lights in dome needs to be equal to the number of images");
}

/*
void ImageSet::initLights() {
	if(light3d) {
		//if dome radius we assume lights are directionals
		if(dome_radius) {
			lights3d = lights;
			//temporary if no better wayto get 3d positions
			for(size_t i = 0; i < lights.size(); i++) {
				lights3d[i] *= dome_radius;
				lights3d[i][2] += vertical_offset;
			}

		} else {
			lights.resize(lights3d.size());
			//we assume lights are NOT directionals and we autodetect a reasonable radius
			//intensity will be corrected using dome_radius as reference in sample
			dome_radius = 0.0f;
			for(size_t i = 0; i < lights3d.size(); i++) {
				float r = lights3d[i].norm();
				dome_radius += r;
				lights[i] = lights3d[i]/r;
			}
			dome_radius /= lights3d.size();
		}
	}
	for(Vector3f &light: lights)
		light.normalize();
}
*/

#ifdef _WIN32
#include <stdio.h>
#else
#include <sys/time.h>
#include <sys/resource.h>
#endif
bool ImageSet::initImages(const char *_path) {
	int noFilesNeeded = images.size() + 50;
#ifdef _WIN32
	int maxfiles = _getmaxstdio();
	if(maxfiles < noFilesNeeded) {
		throw QString("The max number of files that can be opened is: %1.\nIt depends on system security configurations.\nContact us to work on this problem.").arg(maxfiles);
	}
#elif __APPLE__
	struct rlimit limits;
	getrlimit(RLIMIT_NOFILE, &limits);
	if(int(limits.rlim_cur) < noFilesNeeded) {
		throw QString("The max number of files that can be opened is: %1.\nIt depends on system security configurations.\nContact us to work on this problem.").arg(limits.rlim_max);
	}
#elif __linux__
	struct rlimit limits;
	getrlimit(RLIMIT_NOFILE, &limits);
	if(int(limits.rlim_max) < noFilesNeeded) {
		throw QString("The max number of files that can be opened is: %1.\nIt depends on system security configurations.\nContact us to work on this problem.").arg(limits.rlim_max);
	}
	limits.rlim_cur = limits.rlim_max;
	setrlimit(RLIMIT_NOFILE, &limits);
#endif

	QDir dir(_path);
	for(int i = 0; i < images.size(); i++) {
		QString filepath = dir.filePath(images[i]);
		int w, h;
		JpegDecoder *dec = new JpegDecoder;
		if(!dec->init(filepath.toStdString().c_str(), w, h))
			throw QString("Failed decoding image: " + filepath);

		if(width && (width != w || height != h))
			throw QString("Inconsistent image size for " + filepath);

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
			bool keep_going = (*callback)(std::string("Sampling images"), 100*(y-top)/(height-1));
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
	//TODO FIX for crop!;
	assert(width == image_width && height == image_height);
	decoders[img]->readRows(height, buffer);
}


//adjust light for pixel,light is mm coords, return light again in mm. //y is expected with UP axis.
Vector3f ImageSet::relativeLight(const Vector3f &light3d, int x, int y){
	Vector3f l = light3d;
	//relative position to the center in mm
	float dx = pixel_size*(x - image_width/2.0f);
	float dy = pixel_size*(y - image_height/2.0f);
	l[0]  -= dx;
	l[1]  -= dy;
	return l;
}

void ImageSet::readLine(PixelArray &pixels) {
	if(current_line == 0)
		skipToTop();
	pixels.resize(width, images.size());
	for(uint32_t x = 0; x < pixels.size(); x++) {
		Pixel &pixel = pixels[x];
		pixel.x = x + left;
		pixel.y = image_height - 1 - current_line;
	}

	//TODO: no need to allocate EVERY time.
	std::vector<uint8_t> row(image_width*3);

	for(size_t i = 0; i < decoders.size(); i++) {
		decoders[i]->readRows(1, row.data());

		for(int x = left; x < right; x++) {
			pixels[x - left][i].r = row[x*3 + 0];
			pixels[x - left][i].g = row[x*3 + 1];
			pixels[x - left][i].b = row[x*3 + 2];
		}
	}
	if(light3d) {
		compensateIntensity(pixels);
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

uint32_t ImageSet::sample(PixelArray &resample, uint32_t ndimensions, std::function<void(Pixel &, Pixel &)> resampler, uint32_t samplingram) {
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
	PixelArray sample(samplexrow, images.size());

	uint32_t offset = 0;
	vector<uint8_t> row(image_width*3);
	for(int y = top; y < bottom; y++) {
		if(callback && !(*callback)("Sampling images:", 100*(y-top)/(height-1)))
			throw std::string("Cancelled");

		//read one row per image at a time
		auto &selection = sampler.result(samplexrow, width);

		for(uint32_t i = 0; i < decoders.size(); i++) {
			JpegDecoder *dec = decoders[i];
			dec->readRows(1, row.data());
			uint32_t x = 0;
			for(int k: selection) {
				Color3f &pixel = sample[x][i];

				pixel.r = row[(k+left)*3 + 0];
				pixel.g = row[(k+left)*3 + 1];
				pixel.b = row[(k+left)*3 + 2];
				x++;
			}
		}

		if(light3d) {

			uint32_t x = 0;
			for(int k: selection) {
				sample[x].x = k + left;
				sample[x].y = image_height - 1 - y;
				x++;
			}
			compensateIntensity(sample);
		}

		for(uint32_t x = 0; x < selection.size(); x++)
			resampler(sample[x], resample[offset + x]);

		offset += samplexrow;
	}
	return nsamples;
}

void ImageSet::compensateIntensity(PixelArray &pixels) {
	assert(pixel_size != 0.0f);
	assert(lights1.size() == size_t(images.size()));
	assert(lights1.size() == pixels.nlights);
	for(Pixel &pixel: pixels) {
		for(size_t i = 0; i < pixel.size(); i++) {
			Vector3f l = relativeLight(lights1[i], pixel.x, pixel.y);
			float f = l.squaredNorm() / idealLightDistance2;
			pixel[i].r *= f;
			pixel[i].g *= f;
			pixel[i].b *= f;
		}
	}

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
		
		if(callback && !(*callback)("Skipping cropped lines...", 100*i/(decoders.size()-1)))
			throw std::string("Cancelled");
	}
	current_line += top;
}

