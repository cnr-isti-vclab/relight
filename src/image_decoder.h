#ifndef IMAGE_DECODER_H
#define IMAGE_DECODER_H

#include <cstdint>
#include <memory>
#include <vector>

// ──────────────────────────────────────────────────────────────────────────────
// Supported image formats
// ──────────────────────────────────────────────────────────────────────────────
enum class ImageFormat {
	UNKNOWN,
	JPEG,           // libjpeg / libjpeg-turbo
	PNG,            // libpng
	TIFF,           // libtiff
	EXR,            // OpenEXR / TinyEXR
	RAW             // camera RAW via libraw
};

// ──────────────────────────────────────────────────────────────────────────────
// Pixel element type (valid after init() or decode())
// ──────────────────────────────────────────────────────────────────────────────
enum class PixelType {
	UINT8,          //  8 bits per channel  (JPEG, 8-bit PNG/TIFF, processed RAW)
	UINT16,         // 16 bits per channel  (16-bit PNG/TIFF, RAW 16-bit linear)
	FLOAT16,        // half float           (EXR half channels)
	FLOAT32         // 32-bit float         (EXR float, 32-bit TIFF)
};

// ──────────────────────────────────────────────────────────────────────────────
// Abstract per-format backend.
// Concrete implementations (JpegDecoderImpl, PngDecoderImpl, …) live in
// image_decoder.cpp and are never exposed publicly.
// ──────────────────────────────────────────────────────────────────────────────
struct ImageDecoderImpl {
	virtual ~ImageDecoderImpl() = default;

	// Open a file and read its header; sets width and height on success.
	virtual bool open(const char* path, int& width, int& height) = 0;

	// Bytes per complete scanline: width * numChannels() * bytesPerChannel().
	virtual size_t rowSize() const = 0;

	// Read up to `rows` scanlines into `buffer` (uint8_t* raw bytes).
	// Returns the number of rows actually read.
	virtual size_t readRows(int rows, uint8_t* buffer) = 0;

	// Float variant: normalises to [0, 1] for integer formats; copies directly
	// for float formats.  Default implementation converts from the uint8_t path.
	// Format backends that natively produce float (EXR, 32-bit TIFF) should
	// override this for efficiency and override the uint8_t version to quantise.
	virtual size_t readRows(int rows, float* buffer);

	// Release I/O resources.  Should be idempotent.
	virtual bool finish() = 0;

	// Rewind to the first scanline (re-reads from the start of the file).
	virtual bool restart() = 0;

	// Number of channels in the decoded output: 1 (gray), 3 (RGB), 4 (RGBA).
	virtual int       numChannels() const = 0;
	virtual PixelType pixelType()   const = 0;

	// Embedded ICC colour profile.  Returns an empty vector if absent.
	virtual bool                        hasICCProfile() const = 0;
	virtual const std::vector<uint8_t>& getICCProfile() const = 0;
};

// ──────────────────────────────────────────────────────────────────────────────
// ImageDecoder — public API, mirrors JpegDecoder but format-agnostic.
//
// JPEG-specific concepts (colour space, chroma subsampling) are intentionally
// absent; use JpegDecoder directly when you need those details.
// ──────────────────────────────────────────────────────────────────────────────
class ImageDecoder {
public:
	ImageDecoder();
	~ImageDecoder();

	ImageDecoder(const ImageDecoder&) = delete;
	void operator=(const ImageDecoder&) = delete;

	// Detect format from magic bytes (preferred) with extension fallback.
	static ImageFormat detectFormat(const char* path);

	// Override auto-detection.  Call before init() or decode().
	void        setFormat(ImageFormat fmt);
	ImageFormat getFormat() const { return format; }

	// ── Full-image decode ─────────────────────────────────────────────────────
	// Allocates *img with new[]; caller must delete[].
	bool decode(const char* path,         uint8_t*& img, int& width, int& height);
	bool decode(const char* path,         float*&   img, int& width, int& height);

	// In-memory source (JPEG only for now; others will be added with their impls).
	bool decode(uint8_t* buf, size_t len, uint8_t*& img, int& width, int& height);

	// ── Streaming row-by-row API ──────────────────────────────────────────────
	bool   init(const char* path, int& width, int& height);
	size_t rowSize() const;
	size_t readRows(int rows, uint8_t* buffer);
	size_t readRows(int rows, float*   buffer);
	bool   finish();
	bool   restart();

	// ── Pixel format (valid after init() or decode()) ─────────────────────────
	int       numChannels()     const;   // 1, 3, or 4
	PixelType pixelType()       const;
	int       bitsPerChannel()  const;   // 8, 16, or 32
	int       bytesPerChannel() const;   // 1, 2, or 4

	// ── ICC colour profile ────────────────────────────────────────────────────
	bool hasICCProfile() const;
	const std::vector<uint8_t>& getICCProfile() const;

	// Shared empty profile returned by reference when no profile is present.
	static const std::vector<uint8_t> empty_profile;

private:
	bool createImpl(const char* path);

	ImageFormat format = ImageFormat::UNKNOWN;
	std::unique_ptr<ImageDecoderImpl> impl;
};

#endif // IMAGE_DECODER_H
