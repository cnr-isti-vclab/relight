#ifndef IMAGESET_H
#define IMAGESET_H

#include <vector>
#include <string>
#include "relight_vector.h"

#include <QStringList>
#include <functional>
#include <string>

class QJsonObject;
class JpegDecoder;
class QImage;

class ImageSet {
public:
	QString path;
	QStringList images;

	int image_width = 0, image_height = 0; //in pixels original size of the image.
	//current crop: left, top is pixel [0, 0];
	int width = 0, height = 0;
	int left = 0, top = 0, right = 0, bottom = 0;

	//Lights
	std::vector<Vector3f> lights;

	bool light3d = false;
	std::vector<Vector3f> lights3d; //always expressed in cm.

	//Geometry
	float image_width_cm = 0.0f;
	float dome_radius = 0.0f;
	float vertical_offset = 0.0f;

	int current_line = 0;
	
	ImageSet(const char *path = nullptr);
	~ImageSet();

	//light3d if lights are positional instead of directional.
	static void parseLP(QString sphere_path, std::vector<Vector3f> &lights, std::vector<QString> &filenames, int skip_image = -1);
	static void parseJSON(QString filename,  std::vector<Vector3f> &lights);


	bool initFromFolder(const char *path, bool ignore_filenames = true, int skip_image = -1);
	bool initFromProject(QJsonObject &obj, const QString &filename);
	void initLights();
	bool initImages(const char *path); //require lights and images to be available, path points to the dir of the images.
	
	QImage maxImage(std::function<bool(std::string stage, int percent)> *callback = nullptr); 
	void crop(int _left, int _top, int _right, int _bottom);
	void setCallback(std::function<bool(std::string stage, int percent)> *_callback = nullptr) { callback = _callback; }
	//call AFTER initImages and BEFORE breadline, decode or sample.

	void decode(size_t img, unsigned char *buffer);
	void readLine(PixelArray &line);
	uint32_t sample(PixelArray &sample, uint32_t ndimensions, std::function<void(Pixel &, Pixel &)> resampler, uint32_t samplingrate);
	void restart();
	void skipToTop();

	Vector3f relativeLight(const Vector3f &light, int x, int y);
	void saveMean(const char *path, int quality);
	void saveNormals(const char *path); //saves a png.

protected:
	std::function<bool(std::string stage, int percent)> *callback = nullptr;
	std::vector<JpegDecoder *> decoders;
};

#endif // IMAGESET_H
