#ifndef PNG_DECODER_H
#define PNG_DECODER_H

#include "image_decoder.h"

#include <png.h>
#include <cstdio>
#include <string>
#include <vector>

// ══════════════════════════════════════════════════════════════════════════════
// PngDecoderImpl — libpng backend for ImageDecoder
// Supports UINT8 and UINT16; 1 (gray), 2 (gray+alpha), 3 (RGB), 4 (RGBA)
// channels; palette images are expanded to RGB(A); sub-8-bit gray expanded
// to 8-bit; tRNS chunk expanded to an alpha channel.
// 16-bit samples are byte-swapped from PNG's big-endian to native order.
// ICC profile is read from the iCCP chunk.
// ══════════════════════════════════════════════════════════════════════════════
struct PngDecoderImpl : ImageDecoderImpl {
	FILE*       file        = nullptr;
	png_structp png         = nullptr;
	png_infop   info        = nullptr;

	int         width       = 0;
	int         height      = 0;
	int         channels    = 3;
	int         bit_depth   = 8;    // effective depth after transforms (8 or 16)
	int         current_row = 0;
	std::string path_copy;
	std::vector<uint8_t> icc_profile;

	~PngDecoderImpl() override;

	bool   open(const char* path, int& w, int& h) override;
	size_t rowSize()                       const override;
	size_t readRows(int rows, uint8_t* buf)      override;
	// float readRows: inherits the default conversion from ImageDecoderImpl
	bool   finish()                              override;
	bool   restart()                             override;
	int    numChannels()                   const override;
	PixelType pixelType()                  const override;
	bool   hasICCProfile()                 const override;
	const std::vector<uint8_t>& getICCProfile() const override;

private:
	bool initPng(const char* path);
	void cleanupPng();
};

#endif // PNG_DECODER_H
