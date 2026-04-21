#include "png_decoder.h"

#include <cstring>

// ── Helpers ───────────────────────────────────────────────────────────────────

PngDecoderImpl::~PngDecoderImpl() {
	cleanupPng();
}

void PngDecoderImpl::cleanupPng() {
	if (png)
		png_destroy_read_struct(&png, info ? &info : nullptr, nullptr);
	// png_destroy_read_struct sets png (and info, if passed) to nullptr.
	png  = nullptr;
	info = nullptr;
	if (file) { fclose(file); file = nullptr; }
}

// Open (or reopen) the PNG file and read its header.
// All libpng errors are caught via setjmp; on error we clean up and return false.
bool PngDecoderImpl::initPng(const char* path) {
	cleanupPng();

	file = fopen(path, "rb");
	if (!file) return false;

	png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!png) { fclose(file); file = nullptr; return false; }

	info = png_create_info_struct(png);
	if (!info) {
		png_destroy_read_struct(&png, nullptr, nullptr);
		fclose(file); file = nullptr;
		return false;
	}

	// Any libpng error from here on will longjmp back here.
	if (setjmp(png_jmpbuf(png))) {
		cleanupPng();
		return false;
	}

	png_init_io(png, file);
	png_read_info(png, info);

	// Read raw image properties before applying transforms.
	int color_type = int(png_get_color_type(png, info));
	bit_depth      = int(png_get_bit_depth(png, info));

	// ── Transforms ────────────────────────────────────────────────────────────
	// Expand palette → RGB(A)
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png);

	// Expand < 8-bit grayscale to 8-bit
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png);

	// Expand tRNS transparency chunk → alpha channel
	if (png_get_valid(png, info, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png);

	// PNG stores 16-bit values in big-endian order.
	// Swap to native byte order so uint16_t reads work correctly on LE hosts.
	if (bit_depth == 16) {
		const uint16_t probe = 0x0100u;
		if (reinterpret_cast<const uint8_t*>(&probe)[0] == 0x01u)
			png_set_swap(png);  // little-endian host: swap BE → LE
	}

	// Apply transforms and re-read updated info.
	png_read_update_info(png, info);

	width      = int(png_get_image_width(png, info));
	height     = int(png_get_image_height(png, info));
	bit_depth  = int(png_get_bit_depth(png, info));
	color_type = int(png_get_color_type(png, info));

	switch (color_type) {
		case PNG_COLOR_TYPE_GRAY:       channels = 1; break;
		case PNG_COLOR_TYPE_GRAY_ALPHA: channels = 2; break;
		case PNG_COLOR_TYPE_RGB:        channels = 3; break;
		case PNG_COLOR_TYPE_RGB_ALPHA:  channels = 4; break;
		default:                        channels = 3; break;
	}

	// ── ICC profile (iCCP chunk) ───────────────────────────────────────────
	icc_profile.clear();
	{
		png_charp  icc_name          = nullptr;
		int        compression_type  = 0;
		png_bytep  icc_data          = nullptr;
		png_uint_32 icc_len          = 0;
		if (png_get_iCCP(png, info,
		                 &icc_name, &compression_type,
		                 &icc_data, &icc_len) == PNG_INFO_iCCP
		    && icc_data && icc_len > 0)
		{
			icc_profile.assign(icc_data, icc_data + icc_len);
		}
	}

	current_row = 0;
	return true;
}

// ── Public API ────────────────────────────────────────────────────────────────

bool PngDecoderImpl::open(const char* path, int& w, int& h) {
	path_copy = path;
	if (!initPng(path)) return false;
	w = width;
	h = height;
	return true;
}

// Number of native bytes per complete scanline (before 8-bit quantisation).
size_t PngDecoderImpl::rowSize() const {
	return size_t(width) * size_t(channels) * (size_t(bit_depth) / 8u);
}

size_t PngDecoderImpl::readRows(int rows, uint8_t* buf) {
	if (!png || current_row >= height) return 0;
	size_t rs = rowSize();

	// Any libpng error will longjmp here; we return 0 to signal failure.
	if (setjmp(png_jmpbuf(png)))
		return 0;

	int read = 0;
	while (read < rows && current_row < height) {
		png_read_row(png, buf + size_t(read) * rs, nullptr);
		++read;
		++current_row;
	}
	return size_t(read);
}

bool PngDecoderImpl::finish() {
	cleanupPng();
	return true;
}

bool PngDecoderImpl::restart() {
	return initPng(path_copy.c_str());
}

int       PngDecoderImpl::numChannels() const { return channels; }
PixelType PngDecoderImpl::pixelType()   const {
	return (bit_depth <= 8) ? PixelType::UINT8 : PixelType::UINT16;
}
bool PngDecoderImpl::hasICCProfile() const { return !icc_profile.empty(); }
const std::vector<uint8_t>& PngDecoderImpl::getICCProfile() const {
	return icc_profile.empty() ? ImageDecoder::empty_profile : icc_profile;
}
