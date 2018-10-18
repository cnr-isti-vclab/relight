#ifndef IMAGESET_H
#define IMAGESET_H

#include <vector>
#include <string>
#include "vector.h"

#include <QStringList>

class JpegDecoder;

class ImageSet {
public:
	size_t width, height;
	std::vector<Vector3f> lights;
	QStringList images;

	ImageSet(const char *path = NULL);
	~ImageSet();
	bool init(const char *path, bool ignore_filenames = true, int skip_image = -1);

	void decode(size_t img, unsigned char *buffer);
	void readLine(PixelArray &line);
	uint32_t sample(PixelArray &sample, uint32_t samplingrate);
	void restart();

protected:
	std::vector<JpegDecoder *> decoders;
};

#endif // IMAGESET_H
