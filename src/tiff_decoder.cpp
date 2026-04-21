#include "tiff_decoder.h"

#include <algorithm>
#include <cstring>

TiffDecoderImpl::~TiffDecoderImpl() {
	if (tif) { TIFFClose(tif); tif = nullptr; }
}

int TiffDecoderImpl::bytesPerSample() const { return bits / 8; }

bool TiffDecoderImpl::open(const char* path, int& w, int& h) {
	path_copy = path;
	if (tif) { TIFFClose(tif); tif = nullptr; }
	tif = TIFFOpen(path, "r");
	if (!tif) return false;

	uint32_t uw = 0, uh = 0;
	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH,  &uw);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &uh);
	width  = int(uw);
	height = int(uh);
	w = width;
	h = height;

	uint16_t ch = 1, bps = 8, sfmt = SAMPLEFORMAT_UINT;
	TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &ch);
	TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE,   &bps);
	TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLEFORMAT,    &sfmt);
	channels   = int(ch);
	bits       = int(bps);
	sample_fmt = int(sfmt);

	// ICC profile (TIFFTAG_ICCPROFILE = 34675)
	uint32_t icc_len = 0;
	void*    icc_ptr = nullptr;
	if (TIFFGetField(tif, TIFFTAG_ICCPROFILE, &icc_len, &icc_ptr) && icc_ptr && icc_len > 0)
		icc_profile.assign(static_cast<uint8_t*>(icc_ptr),
		                    static_cast<uint8_t*>(icc_ptr) + icc_len);

	tiled = (TIFFIsTiled(tif) != 0);
	if (tiled)
		bufferTiledImage();

	current_row = 0;
	return true;
}

// Assemble tiled TIFF into a contiguous row-major buffer.
void TiffDecoderImpl::bufferTiledImage() {
	uint32_t tile_w = 0, tile_h = 0;
	TIFFGetField(tif, TIFFTAG_TILEWIDTH,  &tile_w);
	TIFFGetField(tif, TIFFTAG_TILELENGTH, &tile_h);

	size_t bps = size_t(bytesPerSample());
	size_t ch  = size_t(channels);
	tile_buf.assign(size_t(width) * size_t(height) * ch * bps, 0);

	std::vector<uint8_t> tmp(size_t(TIFFTileSize(tif)));

	for (uint32_t y0 = 0; y0 < uint32_t(height); y0 += tile_h) {
		for (uint32_t x0 = 0; x0 < uint32_t(width); x0 += tile_w) {
			TIFFReadTile(tif, tmp.data(), x0, y0, 0, 0);
			uint32_t copy_w = std::min(tile_w, uint32_t(width)  - x0);
			uint32_t copy_h = std::min(tile_h, uint32_t(height) - y0);
			for (uint32_t row = 0; row < copy_h; ++row) {
				uint8_t*       dst = tile_buf.data()
				                   + (size_t(y0 + row) * size_t(width) + size_t(x0)) * ch * bps;
				const uint8_t* src = tmp.data()
				                   + size_t(row) * size_t(tile_w) * ch * bps;
				std::memcpy(dst, src, size_t(copy_w) * ch * bps);
			}
		}
	}
}

size_t TiffDecoderImpl::rowSize() const {
	return size_t(width) * size_t(channels) * size_t(bytesPerSample());
}

size_t TiffDecoderImpl::readRows(int rows, uint8_t* buf) {
	if (!tif || current_row >= height) return 0;
	size_t rs   = rowSize();
	int    read = 0;
	if (tiled) {
		while (read < rows && current_row < height) {
			std::memcpy(buf + size_t(read) * rs,
			            tile_buf.data() + size_t(current_row) * rs, rs);
			++read; ++current_row;
		}
	} else {
		while (read < rows && current_row < height) {
			if (TIFFReadScanline(tif, buf + size_t(read) * rs,
			                     uint32_t(current_row), 0) < 0)
				break;
			++read; ++current_row;
		}
	}
	return size_t(read);
}

bool TiffDecoderImpl::finish() {
	if (tif) { TIFFClose(tif); tif = nullptr; }
	tile_buf.clear();
	return true;
}

bool TiffDecoderImpl::restart() {
	current_row = 0;
	if (!tif) {
		// Reopen if finish() was called
		tif = TIFFOpen(path_copy.c_str(), "r");
		if (!tif) return false;
		if (tiled) bufferTiledImage();
	}
	// Non-tiled: TIFFReadScanline accepts any row index, so resetting
	// current_row is sufficient — no seek required.
	return true;
}

int TiffDecoderImpl::numChannels() const { return channels; }

PixelType TiffDecoderImpl::pixelType() const {
	if (sample_fmt == SAMPLEFORMAT_IEEEFP) {
		if (bits == 16) return PixelType::FLOAT16;
		return PixelType::FLOAT32;
	}
	if (bits <= 8) return PixelType::UINT8;
	return PixelType::UINT16;
}

bool TiffDecoderImpl::hasICCProfile() const { return !icc_profile.empty(); }
const std::vector<uint8_t>& TiffDecoderImpl::getICCProfile() const { return icc_profile; }
