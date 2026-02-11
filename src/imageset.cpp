#include "imageset.h"
#include "dome.h"
#include "jpeg_decoder.h"
#include "project.h"
#include "colorprofile.h"
#include "icc_profiles.h"
#include "exif.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>

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
	if(color_transform)
		cmsDeleteTransform(color_transform);
	if(input_profile)
		cmsCloseProfile(input_profile);

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

bool ImageSet::initFromProject(Project &project) {

	vector<bool> visibles;
	for(Image &img: project.images) {
		if(img.skip)
			continue;
		visibles.push_back(img.visible);
		if(img.visible) {
			images.push_back(img.filename);
		}
	}
	lens = project.lens;
	initImages(project.dir.absolutePath().toStdString().c_str());
	initFromDome(project.dome);
	if(lights1.size() != visibles.size()) {
		throw QString("Number of lights in dome needs to be equal to the number of images");
	}
	int count = 0;
	for(size_t i = 0; i < lights1.size(); i++) {
		if(visibles[i])
			lights1[count++] = lights1[i];
	}
	lights1.resize(count);
	return true;
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
	Crop crop;
	if(obj.contains("crop")) {
		QJsonObject c = obj["crop"].toObject();
		crop.setLeft(c["left"].toInt());
		crop.setTop(c["top"].toInt());
		crop.setWidth(c["width"].toInt());
		crop.setHeight(c["height"].toInt());
		if(c.contains("angle"))
			crop.angle = c["angle"].toDouble();
		setCrop(crop);
		//TODO should take int account align values!!!!
	}

	lens.fromJson(obj["lens"].toObject());
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
		setLights(dome.positionsSphere, dome.lightConfiguration);
		break;
	case Dome::LIGHTS3D:
		setLights(dome.positions3d, dome.lightConfiguration);
		break;
	}
}

void ImageSet::setLights(const std::vector<Eigen::Vector3f> &lights, const Dome::LightConfiguration configuration) {
	lights1 = lights;
	light3d = (configuration != Dome::DIRECTIONAL);
	if(light3d) {
		idealLightDistance2 = 0.0f;
		for(auto l: lights1) {
			idealLightDistance2 += l.squaredNorm();
		}
		idealLightDistance2 /= lights1.size();
	}
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
	icc_profile_data.clear();
	bool first = true;
	bool has_profile = false;
	bool is_exif_srgb = false;

	for(int i = 0; i < images.size(); i++) {
		QString filepath = dir.filePath(images[i]);
		int w, h;
		JpegDecoder *dec = new JpegDecoder;
		if(!dec->init(filepath.toStdString().c_str(), w, h))
			throw QString("Failed decoding image: " + filepath);

		if(width && (width != w || height != h)) {
			std::cout << width << " " << w << " h: " << height << " " << h << endl;
			throw QString("Inconsistent image size for " + filepath);
		}

		right = image_width = width = w;
		bottom = image_height = height = h;

		Exif exif;
		exif.parse(filepath);
		lens.readExif(exif);

		if(first) {
			has_profile = dec->hasICCProfile();
			if(has_profile) {
				const auto &profile = dec->getICCProfile();
				icc_profile_data = profile;
			} else
				is_exif_srgb = exif.value(Exif::ColorSpace, QString()).toString() == "sRGB";
			first = false;
		}

		//logic here:
		//Ensure all images have ICCProfile or all have not
		if(dec->hasICCProfile()) {
			if(!has_profile || icc_profile_data != dec->getICCProfile()) {
				throw QString("Input images use different ICC profiles. Mismatch detected in %1")
						.arg(filepath);
			}
		} else {
			if(has_profile)
				throw QString("Input images %1 lacks ICC profile while previous have one.").arg(filepath);
			if(is_exif_srgb != (exif.value(Exif::ColorSpace, QString()).toString() == "sRGB"))
				throw QString("Input image %1 has a different exif colorspace.").arg(filepath);
		}
		decoders.push_back(dec);
	}
	if(is_exif_srgb) {
		icc_profile_data = ICCProfiles::sRGBData();
	}
	return true;
}

void ImageSet::setColorProfileMode(ColorProfileMode mode) {
	if(color_profile_mode == mode)
		return;
	color_profile_mode = mode;
	if(color_transform) {
		cmsDeleteTransform(color_transform);
		color_transform = nullptr;
	}
}


void ImageSet::setCrop(int _left, int _top, int _width, int _height) {
	crop.setRect(QPoint(_left, _top), QSize(_width, _height));
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
	
	uint8_t *row = new uint8_t[w*3];
	
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
			applyColorTransform(row, w);
			
			for(int x = 0; x < image_width; x++) {
				rowmax[x*3 + 0] = std::max(rowmax[x*3 + 0], row[x*3 + 0]);
				rowmax[x*3 + 1] = std::max(rowmax[x*3 + 1], row[x*3 + 1]);
				rowmax[x*3 + 2] = std::max(rowmax[x*3 + 2], row[x*3 + 2]);
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
	applyColorTransform(buffer, size_t(width)*size_t(height));
}


//adjust light for pixel,light is mm coords, return light again in mm. //y is expected with UP axis.
Vector3f ImageSet::relativeLight(const Vector3f &light3d, int x, int y){
	Vector3f l = light3d;
	//relative position to the center in mm

	float dx = pixel_size*(float(x) - image_width/2.0f);
	float dy = pixel_size*(float(y) - image_height/2.0f);
	l[0]  -= dx;
	l[1]  -= dy;

	if(angle != 0.0f) {
		//rotate lights by angle
		float x = l[0]*cos_a - l[1]*sin_a;
		float y = l[0]*sin_a + l[1]*cos_a;
		l[0] = x;
		l[1] = y;
	}
	return l;
}
void ImageSet::compensateVignetting(PixelArray &pixels) {
	if(!lens.focalLength) //this should not really happens.
		return;
	for(Pixel &pixel: pixels) {
		float angle = lens.viewAngle(pixel.x, pixel.y);
		float f = 1/pow(cos(angle), 4);
		for(size_t i = 0; i < pixel.size(); i++) {
			pixel[i].r *= f;
			pixel[i].g *= f;
			pixel[i].b *= f;
		}
	}
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
		applyColorTransform(row.data(), width);
		int x_offset = offsets.size() ? offsets[i].x() : 0;

		for(int x = left; x < right; x++) {
			pixels[x - left][i].r = row[(x + x_offset)*3 + 0];
			pixels[x - left][i].g = row[(x + x_offset)*3 + 1];
			pixels[x - left][i].b = row[(x + x_offset)*3 + 2];
		}
	}
	compensateVignetting(pixels);
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
			applyColorTransform(row.data(), image_width);
			uint32_t x = 0;
			for(int k: selection) {
				Color3f &pixel = sample[x][i];

				pixel.r = row[(k+left)*3 + 0];
				pixel.g = row[(k+left)*3 + 1];
				pixel.b = row[(k+left)*3 + 2];

				x++;
			}
		}
		{
			uint32_t x = 0;
			for(int k: selection) {
				sample[x].x = k + left;
				sample[x].y = image_height - 1 - y;
				x++;
			}
		}
		compensateVignetting(sample);

		if(light3d) {
			compensateIntensity(sample);
		}

		for(uint32_t x = 0; x < selection.size(); x++)
			resampler(sample[x], resample[offset + x]);

		offset += samplexrow;
	}
	return nsamples;
}

void ImageSet::applyColorTransform(uint8_t *data, size_t pixel_count) {
	if(color_profile_mode == COLOR_PROFILE_PRESERVE)
		return;
	if(!color_transform)
		color_transform = ColorProfile::createColorTransform(icc_profile_data, color_profile_mode, input_profile);
	cmsDoTransform(color_transform, data, data, pixel_count);
}

void ImageSet::restart() {
	for(uint32_t i = 0; i < decoders.size(); i++)
		decoders[i]->restart();
	
	current_line = 0;
}

void ImageSet::setCrop(const Crop &_crop) {

	QRect r = _crop.boundingRect(imageSize());
	setCrop(r.left(), r.top(), r.width(), r.height());
	rotateLights(-_crop.angle);
	crop = _crop; //setCrop overwrites this.crop
}

void ImageSet::setCrop(Crop &_crop, const std::vector<QPointF> &_offsets) {
	QRect c = _crop.boundingRect(imageSize());

	std::vector<QPoint> int_offsets;
	for(const QPointF &p: _offsets)
		int_offsets.push_back(p.toPoint());

	//find min and max of offsets to adjust the maxCrop;
	int l = 0;
	int r = image_width;;
	int t = 0;
	int b = image_height;
	for(QPoint &o: int_offsets) {
		l = std::max(l, -o.x());
		r = std::min(r, image_width-o.x());
		t = std::max(t, -o.y());
		b = std::min(b, image_height-o.y());
	}
	//TODO check +1 problem
	QRect max_crop(l, t, r - l, b - t);
	if(c.isNull())
		c = max_crop;
	else
		c = max_crop.intersected(c);

	setCrop(c.left(), c.top(), c.width(), c.height());
	offsets = int_offsets;

	rotateLights(-_crop.angle);
	crop = _crop;
}

void ImageSet::rotateLights(float a) {
	angle = a;
	cos_a = cos(a*M_PI/180.0f);
	sin_a = sin(a*M_PI/180.0f);
	if(!light3d) {
		for(Vector3f &v: lights1) {
			float x = v[0]*cos_a - v[1]*sin_a;
			float y = v[0]*sin_a + v[1]*cos_a;
			v[0] = x;
			v[1] = y;
		}
	}
}

void ImageSet::skipToTop() {
	std::vector<uint8_t> row(image_width*3);

	for(uint32_t i = 0; i < decoders.size(); i++) {
		int y_offset = offsets.size() ? offsets[i].y() : 0;
		for(int y = 0; y < top + y_offset; y++)
			decoders[i]->readRows(1, row.data());
		
		if(callback && !(*callback)("Skipping cropped lines...", 100*i/(decoders.size()-1)))
			throw std::string("Cancelled");
	}
	current_line += top;
}


