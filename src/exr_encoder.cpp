#include "exr_encoder.h"

#include <cstring>
#include <cmath>
#include <cstdlib>

ExrEncoder::ExrEncoder() {

}

ExrEncoder::~ExrEncoder() {
	finish();
}

void ExrEncoder::setColorSpace(int /*dummy*/, int numComponents) {
	this->numComponents = numComponents;
}

int ExrEncoder::getNumComponents() const {
	return numComponents;
}

void ExrEncoder::setQuality(int quality) {
	this->quality = quality;
}

int ExrEncoder::getQuality() const {
	return quality;
}

void ExrEncoder::setDotsPerMeter(float dotsPerMeter) {
	// store in pixels/cm like JpegEncoder (approx)
	dotsPerCM = int(std::round(dotsPerMeter / 100.0f));
}

void ExrEncoder::setICCProfile(const uint8_t* data, size_t length) {
	icc_profile.assign(data, data + length);
}

void ExrEncoder::setICCProfile(const std::vector<uint8_t>& profile) {
	icc_profile = profile;
}

bool ExrEncoder::encode(uint8_t* img, int width, int height, FILE* file) {
	if(!file) return false;
	// Convert and write directly using tinyexr SaveEXR
	// Convert uint8_t [0..255] interleaved to float [0..1]
	int components = numComponents;
	size_t npix = size_t(width) * size_t(height);
	std::vector<float> tmp(npix * size_t(components));
	for(size_t i = 0; i < npix; ++i) {
		for(int c = 0; c < components; ++c) {
			tmp[i * components + c] = float(img[i * components + c]) / 255.0f;
		}
	}

	// tinyexr provides SaveEXR which writes to filename. We have a FILE*; write to temporary memory and fwrite to file.
	unsigned char *mem = nullptr;
	const char *err = nullptr;
	int rc = SaveEXRToMemory(tmp.data(), width, height, components, /*save_as_fp16=*/1, &mem, &err);
	if(rc < 0) {
		if(err) FreeEXRErrorMessage(err);
		if(mem) free(mem);
		return false;
	}

	fwrite(mem, 1, size_t(rc), file);
	free(mem);
	return true;
}

bool ExrEncoder::encode(uint8_t* img, int width, int height, const char* path) {
	if(!path) return false;
	int components = numComponents;
	size_t npix = size_t(width) * size_t(height);
	std::vector<float> tmp(npix * size_t(components));
	for(size_t i = 0; i < npix; ++i) {
		for(int c = 0; c < components; ++c) {
			tmp[i * components + c] = float(img[i * components + c]) / 255.0f;
		}
	}
	const char *err = nullptr;
	int rc = SaveEXR(tmp.data(), width, height, components, /*save_as_fp16=*/1, path, &err);
	if(rc != TINYEXR_SUCCESS) {
		if(err) FreeEXRErrorMessage(err);
		return false;
	}
	return true;
}

bool ExrEncoder::encode(uint8_t* img, int width, int height, uint8_t *&buffer, int &length) {
	int components = numComponents;
	size_t npix = size_t(width) * size_t(height);
	std::vector<float> tmp(npix * size_t(components));
	for(size_t i = 0; i < npix; ++i) {
		for(int c = 0; c < components; ++c) {
			tmp[i * components + c] = float(img[i * components + c]) / 255.0f;
		}
	}

	unsigned char *mem = nullptr;
	const char *err = nullptr;
	int rc = SaveEXRToMemory(tmp.data(), width, height, components, /*save_as_fp16=*/1, &mem, &err);
	if(rc < 0) {
		if(err) FreeEXRErrorMessage(err);
		if(mem) free(mem);
		return false;
	}
	buffer = mem;
	length = rc;
	return true;
}

bool ExrEncoder::init(const char* path, int width, int height) {
	if(!path) return false;
	file = fopen(path, "wb");
	if(!file) return false;
	this->width = width;
	this->height = height;
	float_buf.clear();
	float_buf.reserve(size_t(width) * size_t(height) * size_t(numComponents));
	return true;
}

bool ExrEncoder::init(std::vector<uint8_t> &output, int width, int height) {
	mem_output = &output;
	this->width = width;
	this->height = height;
	float_buf.clear();
	float_buf.reserve(size_t(width) * size_t(height) * size_t(numComponents));
	return true;
}

bool ExrEncoder::writeRows(uint8_t *rows, int n) {
	if(!rows || n <= 0 || width <= 0 || height <= 0) return false;
	int components = numComponents;
	size_t rowSize = size_t(width) * size_t(components);
	for(int r = 0; r < n; ++r) {
		uint8_t *src = rows + size_t(r) * rowSize;
		for(size_t i = 0; i < size_t(width); ++i) {
			for(int c = 0; c < components; ++c) {
				float_buf.push_back(float(src[i * components + c]) / 255.0f);
			}
		}
	}
	return true;
}

size_t ExrEncoder::finish() {
	if(float_buf.empty()) {
		if(file) { fclose(file); file = nullptr; }
		return 0;
	}

	size_t out_size = 0;
	int components = numComponents;
	if(mem_output) {
		unsigned char *mem = nullptr;
		const char *err = nullptr;
		int rc = SaveEXRToMemory(float_buf.data(), width, height, components, /*save_as_fp16=*/1, &mem, &err);
		if(rc > 0) {
			mem_output->assign(mem, mem + rc);
			out_size = size_t(rc);
		}
		if(err) FreeEXRErrorMessage(err);
		if(mem) free(mem);
		mem_output = nullptr;
	} else if(file) {
		unsigned char *mem = nullptr;
		const char *err = nullptr;
		int rc = SaveEXRToMemory(float_buf.data(), width, height, components, /*save_as_fp16=*/1, &mem, &err);
		if(rc > 0) {
			fwrite(mem, 1, size_t(rc), file);
			out_size = size_t(rc);
		}
		if(err) FreeEXRErrorMessage(err);
		if(mem) free(mem);
		fclose(file);
		file = nullptr;
	}

	float_buf.clear();
	float_buf.shrink_to_fit();
	return out_size;
}
