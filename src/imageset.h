#ifndef IMAGESET_H
#define IMAGESET_H

#include <vector>
#include <string>
#include "vector.h"

#include <QStringList>
#include <functional>
#include <string>

class JpegDecoder;

class ImageSet {
public:
	int width, height;
	int image_width, image_height;
	//left, top is pixel [0, 0];
	int left = 0, top = 0, right = 0, bottom = 0;
	std::vector<Vector3f> lights;
	QStringList images;

	ImageSet(const char *path = NULL);
	~ImageSet();

	bool init(const char *path, bool ignore_filenames = true, int skip_image = -1);
	bool initImages(const char *path); //require lights and images to be available, path points to the dir of the images.
	void crop(int _left, int _top, int _right, int _bottom);
	//call AFTER initImages and BEFORE breadline, decode or sample.

	void decode(size_t img, unsigned char *buffer);
	void readLine(PixelArray &line);
	uint32_t sample(PixelArray &sample, uint32_t samplingrate,  std::function<void(std::string stage, int percent)> *callback = nullptr);
	void restart();
	void skipToTop();

protected:
	std::vector<JpegDecoder *> decoders;
};

#endif // IMAGESET_H
