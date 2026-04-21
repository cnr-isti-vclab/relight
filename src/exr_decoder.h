#ifndef EXR_DECODER_H
#define EXR_DECODER_H

#include "image_decoder.h"

#include <string>
#include <vector>

// Forward-declare tinyexr structs so the header doesn't pull in tinyexr.h
// (which contains the full implementation when TINYEXR_IMPLEMENTATION is defined).
struct TEXRHeader;
struct TEXRImage;

// ══════════════════════════════════════════════════════════════════════════════
// ExrDecoderImpl — tinyexr backend for ImageDecoder
//
// EXR is always scene-linear; no embedded ICC profile.
// All channel types (HALF, FLOAT, UINT) are converted to FLOAT32 on load.
// Channel layout detection tries R/G/B[/A] by name (handles layered EXRs
// like "diffuse.R"), falls back to luminance (Y), then to all channels as-is.
// The full image is decoded on open() into an interleaved float buffer so that
// readRows() can serve rows cheaply without re-parsing.
// ══════════════════════════════════════════════════════════════════════════════
struct ExrDecoderImpl : ImageDecoderImpl {
	int  width       = 0;
	int  height      = 0;
	int  channels    = 0;
	int  current_row = 0;
	std::string path_copy;

	// Interleaved row-major float buffer: [height][width][channels]
	std::vector<float> img_buf;

	// Mapping from output channel index → EXR channel index in the loaded image
	std::vector<int>   channel_map;

	~ExrDecoderImpl() override = default;

	bool   open(const char* path, int& w, int& h) override;
	size_t rowSize()                       const override;
	// uint8_t path: never called directly — ImageDecoder::readRows(uint8_t*)
	// handles quantisation via the float path below.
	size_t readRows(int rows, uint8_t* buf)      override;
	// Float path reads directly from the interleaved buffer (no conversion needed).
	size_t readRows(int rows, float*   buf)      override;
	bool   finish()                              override;
	bool   restart()                             override;
	int    numChannels()                   const override;
	PixelType pixelType()                  const override;
	bool   hasICCProfile()                 const override;
	const std::vector<uint8_t>& getICCProfile() const override;

private:
	bool loadImage(const char* path);
};

#endif // EXR_DECODER_H
