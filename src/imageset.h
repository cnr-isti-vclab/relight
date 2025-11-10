#ifndef IMAGESET_H
#define IMAGESET_H

#include "relight_vector.h"
#include "dome.h"
#include <vector>
#include <string>
#include <functional>
#include <string>

#include <Eigen/Core>
#include <QStringList>
#include <QPoint>
#include <QSize>

class QJsonObject;
class JpegDecoder;
class QImage;
class QRect;
class Image;
class Project;
class Crop;

class ImageSet {
public:
	QString path;
	QStringList images;
	std::vector<Eigen::Vector3f> lights1;

	int image_width = 0, image_height = 0; //in pixels original size of the image.
	//current crop: left, top is pixel [0, 0];
	int width = 0, height = 0;
	int left = 0, top = 0, right = 0, bottom = 0;
	float angle = 0.0f; //
	float cos_a = 1.0f;
	float sin_a = 0.0f;

	float pixel_size = 0;

	bool light3d = false;
	float idealLightDistance2 = 0.0f; //squared, used when rescaling intensity

	int current_line = 0;
	std::vector<QPoint> offsets; //align offsets
	
	ImageSet(const char *path = nullptr);
	~ImageSet();

	//light3d if lights are positional instead of directional.
	//static void parseLP(QString sphere_path, std::vector<Eigen::Vector3f> &lights, std::vector<QString> &filenames, int skip_image = -1);
	static void parseJSON(QString filename,  std::vector<Eigen::Vector3f> &lights);


	//find images in a folder (calls initimages)
	bool initFromFolder(const char *path, int skip_image = -1);
	//load light configuration from dome.
	void initFromDome(Dome &dome);

	//find images in a project, set crop, loads light configuration in dome.
	bool initFromProject(QJsonObject &obj, const QString &filename);

	bool initFromProject(Project &project);

	//open images and starts the decoders
	bool initImages(const char *path); //path points to the dir of the images.

	//remove not visible images and relative lights.
	void cleanHidden(std::vector<Image> &images);

	void setCrop(int _left, int _top, int _right, int _bottom);
	void setCrop(Crop &crop);
	void setCrop(Crop &crop, const std::vector<QPointF> &offsets);
	void rotateLights(float angle);



	void setLights(const std::vector<Eigen::Vector3f> &lights, const Dome::LightConfiguration configuration);
	std::vector<Eigen::Vector3f> &lights() { return lights1; }

	size_t size() { return size_t(images.size()); }

	QImage maxImage(std::function<bool(std::string stage, int percent)> *callback = nullptr); 
	QSize imageSize() { return QSize(image_width, image_height); }

	void setCallback(std::function<bool(QString stage, int percent)> *_callback = nullptr) { callback = _callback; }
	//call AFTER initImages and BEFORE breadline, decode or sample.

	void decode(size_t img, unsigned char *buffer);
	void readLine(PixelArray &line);
	uint32_t sample(PixelArray &sample, uint32_t ndimensions, std::function<void(Pixel &, Pixel &)> resampler, uint32_t samplingrate);
	void restart();
	void skipToTop();

	Eigen::Vector3f relativeLight(const Eigen::Vector3f &light, int x, int y);

protected:
	std::function<bool(QString stage, int percent)> *callback;
	std::vector<JpegDecoder *> decoders;

private:

	void compensateIntensity(PixelArray &pixels);
};

#endif // IMAGESET_H
