#include "jpeg_encoder.h"

#include <cmath>
#include <iostream>
using namespace std;

JpegEncoder::JpegEncoder() {
	info.err = jpeg_std_error(&errMgr);
	jpeg_create_compress(&info);
}

JpegEncoder::~JpegEncoder() {
	jpeg_destroy_compress(&info);
}

void JpegEncoder::setColorSpace(J_COLOR_SPACE colorSpace, int numComponents) {
	this->colorSpace = colorSpace;
	this->numComponents = numComponents;
}

void JpegEncoder::setJpegColorSpace(J_COLOR_SPACE colorSpace) {
	this->jpegColorSpace = colorSpace;
}

J_COLOR_SPACE JpegEncoder::getColorSpace() const {
	return colorSpace;
}

int JpegEncoder::getNumComponents() const {
	return numComponents;
}

void JpegEncoder::setQuality(int quality) {
	this->quality = quality;
}

int JpegEncoder::getQuality() const {
	return quality;
}

void JpegEncoder::setOptimize(bool optimize) {
	this->optimize = optimize;
}

void JpegEncoder::setChromaSubsampling(bool subsample) {
	this->subsample = subsample;
}

void JpegEncoder::setDotsPerMeter(float dotsPerMeter) {
	this->dotsPerCM = round( dotsPerMeter / 100.0 );  // JPEG requires a resolution in pixels/cm
}


bool JpegEncoder::encode(uint8_t* img, int width, int height, FILE* file) {
	if (file == nullptr)
		return false;

	jpeg_stdio_dest(&info, file);
	return encode(img, width, height);
}

bool JpegEncoder::encode(uint8_t* img, int width, int height, const char* path) {
	FILE* file = fopen(path, "wb");
	if(!file) return false;
	bool rv = encode(img, width, height, file);
	fclose(file);
	return rv;
}

bool JpegEncoder::encode(uint8_t* img, int width, int height, uint8_t *&buffer, int &length) {
	unsigned char *mem = nullptr;
	unsigned long mem_size = 0;
	jpeg_mem_dest(&info, &mem, &mem_size);
	bool ok = encode(img, width, height);
	length = mem_size;
	if(!ok) return false;
	buffer = mem;
	return true;
}

bool JpegEncoder::encode(uint8_t* img, int width, int height) {
	info.image_width = width;
	info.image_height = height;
	info.in_color_space = colorSpace;
	info.input_components = numComponents;

	jpeg_set_defaults(&info);
	jpeg_set_colorspace(&info, jpegColorSpace);
	jpeg_set_quality(&info, quality, (boolean)true);
	info.optimize_coding = (boolean)optimize;

	// Set our output resolution if provided in pixels/cm
	if(dotsPerCM > 0) {
		info.X_density = dotsPerCM;
		info.Y_density = dotsPerCM;
		info.density_unit = 2;   // 2 = pixels per cm
	}

	if(jpegColorSpace == JCS_YCbCr && subsample == false) {
		for(int i = 0; i < numComponents; i++) {
			info.comp_info[i].h_samp_factor = 1;
			info.comp_info[i].v_samp_factor = 1;
		}
	}

	jpeg_start_compress(&info, (boolean)true);

	writeRows(img, height);
/*	int rowSize = info.image_width * info.input_components;
	while (info.next_scanline < info.image_height) {
		JSAMPROW row = img + info.next_scanline * rowSize;
		jpeg_write_scanlines(&info, &row, 1);
	} */

	jpeg_finish_compress(&info);
	return true;
}


bool JpegEncoder::init(const char* path, int width, int height) {
	file = fopen(path, "wb");
	if(!file)
		return false;
	jpeg_stdio_dest(&info, file);
	return init(width, height);
}

bool JpegEncoder::init(int width, int height) {
	info.image_width = width;
	info.image_height = height;
	info.in_color_space = colorSpace;
	info.input_components = numComponents;

	jpeg_set_defaults(&info);
	jpeg_set_colorspace(&info, jpegColorSpace);
	jpeg_set_quality(&info, quality, (boolean)true);
	info.optimize_coding = (boolean)optimize;

	// Set our output resolution if provided in pixels/cm
	if(dotsPerCM>0) {
	        info.X_density = dotsPerCM;
		info.Y_density = dotsPerCM;
		info.density_unit = 2;   // 2 = pixels per cm
	}

	if(jpegColorSpace == JCS_YCbCr && subsample == false)
		for(int i = 0; i < numComponents; i++) {
			info.comp_info[i].h_samp_factor = 1;
			info.comp_info[i].v_samp_factor = 1;
		}

	jpeg_start_compress(&info, (boolean)true);
	return true;
}

bool JpegEncoder::writeRows(uint8_t *rows, int n) {
	int written = 0;
	int rowSize = info.image_width * info.input_components;

	while (info.next_scanline < info.image_height && written < n) {
		JSAMPROW row = rows + written * rowSize;
		jpeg_write_scanlines(&info, &row, 1);
		written++;
	}
	return true;
}

size_t JpegEncoder::finish() {
	jpeg_finish_compress(&info);
	size_t size = 0;
	if(file) {
		size = ftell(file);
		fclose(file);
	}
	return size;
}

void JpegEncoder :: onError(j_common_ptr /* cinfo */)
{
/*	// cinfo->err is actually a pointer to my_error_mgr.defaultErrorManager, since pub
	// is the first element of my_error_mgr we can do a sneaky cast
	ErrorManager* pErrorManager = (ErrorManager*) cinfo->err;
	(*cinfo->err->output_message)(cinfo); // print error message (actually disabled below)
	longjmp(cinfo->err->jumpBuffer, 1); */
}


void JpegEncoder :: onMessage(j_common_ptr /* cinfo */)
{
	// disable error messages
	/*char buffer[JMSG_LENGTH_MAX];
	(*cinfo->err->format_message) (cinfo, buffer);
	fprintf(stderr, "%s\n", buffer);*/
}



