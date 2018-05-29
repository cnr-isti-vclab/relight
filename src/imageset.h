#ifndef IMAGESET_H
#define IMAGESET_H

#include <vector>
#include <string>
#include "vector.h"

class JpegDecoder;

class ImageSet {
public:
	size_t width, height;
	std::vector<Vector3f> lights;

	ImageSet(const char *path = NULL);
	~ImageSet();
	bool init(const char *path, bool ignore_filenames = true);

	void decode(size_t img, unsigned char *buffer);
	void readLine(PixelArray &line);
	uint32_t sample(PixelArray &sample, uint32_t samplingrate);
	void restart();

protected:
	std::vector<JpegDecoder *> decoders;
};

#endif // IMAGESET_H
