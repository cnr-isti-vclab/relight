#ifndef JPEGENCODER_H_
#define JPEGENCODER_H_

#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <vector>

#include <jpeglib.h>

class JpegEncoder {
public:
	JpegEncoder();
	~JpegEncoder();

	JpegEncoder(const JpegEncoder&) = delete;
	void operator=(const JpegEncoder&) = delete;

	// JCS_GRAYSCALE, JCS_RGB, JCS_YCbCr, or JCS_CMYK.
	void setColorSpace(J_COLOR_SPACE colorSpace, int numComponents);
	void setJpegColorSpace(J_COLOR_SPACE colorSpace);
	J_COLOR_SPACE getColorSpace() const;
	int getNumComponents() const;
	void setQuality(int quality);
	int getQuality() const;
	void setOptimize(bool optimize);
	void setChromaSubsampling(bool subsample);
	void setDotsPerMeter(float dotsPerMeter);

	// ICC color profile support
	void setICCProfile(const uint8_t* data, size_t length);
	void setICCProfile(const std::vector<uint8_t>& profile);
	bool hasICCProfile() const { return !icc_profile.empty(); }

	bool encode(uint8_t *img, int width, int height, FILE* file);
	bool encode(uint8_t *img, int width, int height, const char* path);
	bool encode(uint8_t *img, int width, int height, uint8_t *&buffer, int &length);

	bool init(const char* path, int width, int height);
	bool writeRows(uint8_t *rows, int n);
	size_t finish(); //return size

private:
	bool init(int width, int height);
	bool encode(uint8_t* img, int width, int height);
	void writeICCProfile();
	static void onError(j_common_ptr cinfo);
	static void onMessage(j_common_ptr cinfo);

	FILE * file = nullptr;
	jpeg_compress_struct info;
	jpeg_error_mgr errMgr;

	J_COLOR_SPACE colorSpace = JCS_RGB;
	J_COLOR_SPACE  jpegColorSpace = JCS_YCbCr;
	int numComponents = 3;
	bool optimize = true;
	bool subsample = false;

	int quality = 95;
	int dotsPerCM = 0;
	std::vector<uint8_t> icc_profile;
};

#endif // JPEGENCODER_H_
