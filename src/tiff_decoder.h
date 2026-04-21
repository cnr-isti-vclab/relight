#ifndef TIFF_DECODER_H
#define TIFF_DECODER_H

#include "image_decoder.h"

#include <tiffio.h>
#include <string>
#include <vector>

// ══════════════════════════════════════════════════════════════════════════════
// TiffDecoderImpl — libtiff backend for ImageDecoder
// Supports UINT8, UINT16, FLOAT16, FLOAT32; 1/3/4 channels; PLANARCONFIG_CONTIG.
// Tiled files are buffered fully on open(); strip/scanline files are streamed
// row-by-row via TIFFReadScanline (which supports random row access).
// ICC profile is read from tag 34675 (TIFFTAG_ICCPROFILE).
// ══════════════════════════════════════════════════════════════════════════════
struct TiffDecoderImpl : ImageDecoderImpl {
	TIFF*       tif         = nullptr;
	std::string path_copy;
	int         width       = 0;
	int         height      = 0;
	int         channels    = 1;
	int         bits        = 8;               // bits per sample
	int         sample_fmt  = SAMPLEFORMAT_UINT;
	int         current_row = 0;
	bool        tiled       = false;
	std::vector<uint8_t> tile_buf;             // full image buffer for tiled files
	std::vector<uint8_t> icc_profile;

	~TiffDecoderImpl() override;

	int    bytesPerSample() const;
	bool   open(const char* path, int& w, int& h) override;
	void   bufferTiledImage();
	size_t rowSize()                     const override;
	size_t readRows(int rows, uint8_t* buf)    override;
	// float readRows: inherits the default conversion from ImageDecoderImpl
	bool   finish()                            override;
	bool   restart()                           override;
	int    numChannels()                 const override;
	PixelType pixelType()                const override;
	bool   hasICCProfile()               const override;
	const std::vector<uint8_t>& getICCProfile() const override;
};

#endif // TIFF_DECODER_H
