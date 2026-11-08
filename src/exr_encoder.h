#ifndef EXRENCODER_H_
#define EXRENCODER_H_

#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>

#include "tinyexr.h"

class ExrEncoder {
public:
	ExrEncoder();
	~ExrEncoder();

	ExrEncoder(const ExrEncoder&) = delete;
	void operator=(const ExrEncoder&) = delete;

	// Mirrors JpegEncoder API where sensible
	void setColorSpace(int /*dummy*/, int numComponents);
	int getNumComponents() const;
	void setQuality(int quality); // maps loosely to compression preference
	int getQuality() const;
	void setDotsPerMeter(float dotsPerMeter);

	// ICC color profile support (EXR typically doesn't embed ICC, stored but unused)
	void setICCProfile(const uint8_t* data, size_t length);
	void setICCProfile(const std::vector<uint8_t>& profile);
	bool hasICCProfile() const { return !icc_profile.empty(); }

	bool encode(uint8_t *img, int width, int height, FILE* file);
	bool encode(uint8_t *img, int width, int height, const char* path);
	bool encode(uint8_t *img, int width, int height, uint8_t *&buffer, int &length);

	bool init(const char* path, int width, int height);
	bool init(std::vector<uint8_t> &output, int width, int height);
	bool writeRows(uint8_t *rows, int n);
	size_t finish(); //return size

private:
	int width = 0;
	int height = 0;
	int numComponents = 3;
	int quality = 0;
	int dotsPerCM = 0;
	std::vector<uint8_t> icc_profile;

	// For file or memory output
	FILE *file = nullptr;
	std::vector<uint8_t> *mem_output = nullptr;

	// Accumulated pixel buffer in float (interleaved RGB[A])
	std::vector<float> float_buf;
};

#endif // EXRENCODER_H_
