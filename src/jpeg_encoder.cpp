#include "jpeg_encoder.h"

#include <cmath>
#include <cstring>
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

void JpegEncoder::setICCProfile(const uint8_t* data, size_t length) {
	icc_profile.assign(data, data + length);
}

void JpegEncoder::setICCProfile(const std::vector<uint8_t>& profile) {
	icc_profile = profile;
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

	// Write ICC profile if present
	writeICCProfile();

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
	
	// Write ICC profile if present
	writeICCProfile();
	
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

void JpegEncoder::writeICCProfile() {
	if (icc_profile.empty()) {
		return;
	}
	
	// ICC profile is written as one or more APP2 markers
	// Each marker: "ICC_PROFILE\0" + seq_no + num_markers + data
	// Maximum data per marker is 65533 bytes (65535 - 2 for marker length field)
	const unsigned int MAX_BYTES_PER_MARKER = 65533 - 14; // 14 = header size
	const unsigned int num_markers = (icc_profile.size() + MAX_BYTES_PER_MARKER - 1) / MAX_BYTES_PER_MARKER;
	
	size_t offset = 0;
	for (unsigned int marker_num = 1; marker_num <= num_markers; marker_num++) {
		size_t length = std::min(MAX_BYTES_PER_MARKER, (unsigned int)(icc_profile.size() - offset));
		
		// Build marker data
		std::vector<uint8_t> marker_data(14 + length);
		std::memcpy(&marker_data[0], "ICC_PROFILE\0", 12);
		marker_data[12] = marker_num;
		marker_data[13] = num_markers;
		std::memcpy(&marker_data[14], &icc_profile[offset], length);
		
		// Write APP2 marker
		jpeg_write_marker(&info, JPEG_APP0 + 2, marker_data.data(), marker_data.size());
		
		offset += length;
	}
}



