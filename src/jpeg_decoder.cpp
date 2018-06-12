#include "jpeg_decoder.h"

JpegDecoder::JpegDecoder() {
	decInfo.err = jpeg_std_error(&errMgr);
	jpeg_create_decompress(&decInfo);
}

JpegDecoder::~JpegDecoder() {
	if(file)
		fclose(file);
	jpeg_destroy_decompress(&decInfo);
}

void JpegDecoder::setColorSpace(J_COLOR_SPACE colorSpace) {
	this->colorSpace = colorSpace;
}

void JpegDecoder::setJpegColorSpace(J_COLOR_SPACE colorSpace) {
	this->jpegColorSpace = colorSpace;
}

J_COLOR_SPACE JpegDecoder::getColorSpace() const {
	return colorSpace;
}

bool JpegDecoder::decode(uint8_t* buffer, size_t len, uint8_t*& img, int& width, int& height) {
	if (buffer == nullptr)
		return false;

	jpeg_mem_src(&decInfo, buffer, len);
	return decode(img, width, height);
}

bool JpegDecoder::decode(const char* path, uint8_t*& img, int& width, int& height) {
	FILE* file = fopen(path, "rb");
	if(!file) return false;
	jpeg_stdio_src(&decInfo, file);
	bool rv = decode(img, width, height);
	fclose(file);
	file = NULL;
	return rv;
}


bool JpegDecoder::decode(FILE *file, uint8_t*& img, int& width, int& height) {
	jpeg_stdio_src(&decInfo, file);
	return decode(img, width, height);
}


bool JpegDecoder::decode(uint8_t*& img, int& width, int& height) {
	init(width, height);

	img = new uint8_t[decInfo.image_height * rowSize()];

	int readed = readRows(height, img);
	if(readed != height)
		return false;
/*	JSAMPROW rows[1];
	size_t offset = 0;
	while (decInfo.output_scanline < decInfo.image_height) {
		rows[0] = img + offset;
		jpeg_read_scanlines(&decInfo, rows, 1);
		offset += rowSize;
	} */

//	jpeg_finish_decompress(&decInfo);
	return true;
}

bool JpegDecoder::init(const char* path, int &width, int &height) {
	file = fopen(path, "rb");
	if(!file) return false;
	jpeg_stdio_src(&decInfo, file);
	return init(width, height);
}

bool JpegDecoder::init(int &width, int &height) {
	jpeg_read_header(&decInfo, true);
	decInfo.out_color_space = colorSpace;
	decInfo.jpeg_color_space = jpegColorSpace;
	decInfo.raw_data_out = false;
	
	if(decInfo.num_components > 1) 
		subsampled =  decInfo.comp_info[1].h_samp_factor != 1;

	jpeg_start_decompress(&decInfo);

	width = decInfo.image_width;
	height = decInfo.image_height;
	return true;
}

size_t JpegDecoder::readRows(int nrows, uint8_t *buffer) { //return false on end.
	if(decInfo.output_scanline == decInfo.image_height)
		restart();

	size_t rowSize = decInfo.image_width * decInfo.num_components;
	JSAMPROW rows[1];
	size_t offset = 0;
	int readed = 0;
	while (decInfo.output_scanline < decInfo.image_height && readed < nrows) {
		readed++;
		rows[0] = buffer + offset;
		jpeg_read_scanlines(&decInfo, rows, 1);
		offset += rowSize;
	}

	if(decInfo.output_scanline == decInfo.image_height)
		jpeg_finish_decompress(&decInfo);
	return readed;
}

bool JpegDecoder::finish() {
	if(file)
		fclose(file);
	return jpeg_finish_decompress(&decInfo);
}

bool JpegDecoder::restart() {
	//jpeg_finish_decompress(&decInfo);
	rewind(file);
	jpeg_stdio_src(&decInfo, file);
	int w, h;
	return init(w, h);
}


