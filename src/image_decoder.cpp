#include "image_decoder.h"
#include "jpeg_decoder.h"

#include "tiff_decoder.h"
#include "png_decoder.h"
#include "exr_decoder.h"

// Format-specific includes -- uncomment as each backend is implemented:
// #include <libraw/libraw.h>

#include <algorithm>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <string>
#include <vector>

// ──────────────────────────────────────────────────────────────────────────────
// Half-float (IEEE 754-2008) → float helper
// ──────────────────────────────────────────────────────────────────────────────
static float half_to_float(uint16_t h) {
	uint32_t sign     = uint32_t(h >> 15) << 31;
	uint32_t exponent = (h >> 10) & 0x1fu;
	uint32_t mantissa =  h        & 0x3ffu;

	uint32_t bits;
	if (exponent == 0) {
		if (mantissa == 0) {
			bits = sign;                                            // ±zero
		} else {
			// Subnormal half → normalised float
			exponent = 1;
			while (!(mantissa & 0x400u)) { mantissa <<= 1; --exponent; }
			mantissa &= 0x3ffu;
			bits = sign | ((exponent + 112u) << 23) | (mantissa << 13);
		}
	} else if (exponent == 31u) {
		bits = sign | (0xffu << 23) | (mantissa << 13);            // Inf / NaN
	} else {
		bits = sign | ((exponent + 112u) << 23) | (mantissa << 13);
	}
	float f;
	std::memcpy(&f, &bits, sizeof f);
	return f;
}

// ──────────────────────────────────────────────────────────────────────────────
// ImageDecoderImpl — default float readRows
// Reads native bytes, converts to [0,1] float (or copies for float formats).
// Format backends that produce float natively should override this method.
// ──────────────────────────────────────────────────────────────────────────────
size_t ImageDecoderImpl::readRows(int rows, float* buffer) {
	std::vector<uint8_t> tmp(size_t(rows) * rowSize());
	size_t read        = readRows(rows, tmp.data());
	size_t total_bytes = read * rowSize();

	switch (pixelType()) {
	case PixelType::UINT8: {
		size_t n = total_bytes;
		for (size_t i = 0; i < n; ++i)
			buffer[i] = tmp[i] * (1.0f / 255.0f);
		break;
	}
	case PixelType::UINT16: {
		size_t           n   = total_bytes / 2;
		const uint16_t* src  = reinterpret_cast<const uint16_t*>(tmp.data());
		for (size_t i = 0; i < n; ++i)
			buffer[i] = src[i] * (1.0f / 65535.0f);
		break;
	}
	case PixelType::FLOAT16: {
		size_t           n   = total_bytes / 2;
		const uint16_t* src  = reinterpret_cast<const uint16_t*>(tmp.data());
		for (size_t i = 0; i < n; ++i)
			buffer[i] = half_to_float(src[i]);
		break;
	}
	case PixelType::FLOAT32: {
		size_t n = total_bytes / 4;
		std::memcpy(buffer, tmp.data(), n * sizeof(float));
		break;
	}
	}
	return read;
}

// ══════════════════════════════════════════════════════════════════════════════
// JpegDecoderImpl — wraps the existing JpegDecoder
// ══════════════════════════════════════════════════════════════════════════════
struct JpegDecoderImpl : ImageDecoderImpl {
	JpegDecoder dec;
	int img_width = 0;     // stored after open() so numChannels() can be computed

	bool open(const char* path, int& w, int& h) override {
		bool ok = dec.init(path, w, h);
		if (ok) img_width = w;
		return ok;
	}

	size_t rowSize() const override {
		return dec.rowSize();
	}

	size_t readRows(int rows, uint8_t* buf) override {
		return dec.readRows(rows, buf);
	}
	// float readRows: inherits the default uint8→float conversion from ImageDecoderImpl

	bool finish()  override { return dec.finish();  }
	bool restart() override { return dec.restart(); }

	int numChannels() const override {
		// rowSize() = width * num_components; derive channels without touching internals.
		return (img_width > 0) ? int(dec.rowSize() / size_t(img_width)) : 0;
	}
	PixelType pixelType() const override { return PixelType::UINT8; }

	bool hasICCProfile() const override { return dec.hasICCProfile(); }
	const std::vector<uint8_t>& getICCProfile() const override {
		return dec.getICCProfile();
	}
};

// ══════════════════════════════════════════════════════════════════════════════
// RawDecoderImpl  — stub (TODO: implement with LibRaw)
// LibRaw decodes the whole image at once; open() should perform the full
// demosaicing and store the result in an internal buffer so that readRows()
// can stream from it row by row.
// Output: 3-channel RGB, UINT16 (linear, white-balance applied).
// ══════════════════════════════════════════════════════════════════════════════
struct RawDecoderImpl : ImageDecoderImpl {
	// TODO: LibRaw raw;
	// TODO: std::vector<uint16_t> internal_buf;
	// TODO: int width = 0, height = 0;
	// TODO: int current_row = 0;

	bool   open(const char*, int&, int&) override { return false; /* TODO */ }
	size_t rowSize()             const override { return 0; }
	size_t readRows(int, uint8_t*) override    { return 0; }
	bool   finish()                    override { return false; }
	bool   restart()                   override { return false; }
	int       numChannels() const override { return 3; }   // always RGB after demosaicing
	PixelType pixelType()   const override { return PixelType::UINT16; }
	bool hasICCProfile() const override { return false; }
	const std::vector<uint8_t>& getICCProfile() const override {
		return ImageDecoder::empty_profile;
	}
};

// ──────────────────────────────────────────────────────────────────────────────
// ImageDecoder
// ──────────────────────────────────────────────────────────────────────────────

const std::vector<uint8_t> ImageDecoder::empty_profile;

ImageDecoder::ImageDecoder()  = default;
ImageDecoder::~ImageDecoder() = default;

// ── Format detection ──────────────────────────────────────────────────────────

static std::string lowerExtension(const char* path) {
	std::string s(path);
	auto pos = s.rfind('.');
	if (pos == std::string::npos) return {};
	std::string ext = s.substr(pos + 1);
	for (char& c : ext) c = char(std::tolower(unsigned(c)));
	return ext;
}

/*static*/
ImageFormat ImageDecoder::detectFormat(const char* path) {
	// 1. Magic bytes — more reliable than extension
	if (FILE* f = fopen(path, "rb")) {
		uint8_t magic[8] = {};
		size_t  n = fread(magic, 1, sizeof magic, f);
		fclose(f);

		if (n >= 2 && magic[0] == 0xff && magic[1] == 0xd8)
			return ImageFormat::JPEG;

		if (n >= 8 && magic[0] == 0x89 && magic[1] == 'P'
		           && magic[2] == 'N'  && magic[3] == 'G')
			return ImageFormat::PNG;

		if (n >= 4 && ((magic[0]=='I' && magic[1]=='I' && magic[2]==0x2a && magic[3]==0x00) ||
		               (magic[0]=='M' && magic[1]=='M' && magic[2]==0x00 && magic[3]==0x2a)))
			return ImageFormat::TIFF;

		// OpenEXR magic: 0x762f3101
		if (n >= 4 && magic[0]==0x76 && magic[1]==0x2f && magic[2]==0x31 && magic[3]==0x01)
			return ImageFormat::EXR;
	}

	// 2. Extension fallback
	std::string ext = lowerExtension(path);
	if (ext == "jpg"  || ext == "jpeg")          return ImageFormat::JPEG;
	if (ext == "png")                            return ImageFormat::PNG;
	if (ext == "tif"  || ext == "tiff")          return ImageFormat::TIFF;
	if (ext == "exr")                            return ImageFormat::EXR;
	if (ext == "cr2"  || ext == "cr3"  ||
	    ext == "nef"  || ext == "arw"  ||
	    ext == "dng"  || ext == "raf"  ||
	    ext == "orf"  || ext == "rw2")           return ImageFormat::RAW;

	return ImageFormat::UNKNOWN;
}

void ImageDecoder::setFormat(ImageFormat fmt) { format = fmt; }

/*static*/
const std::vector<std::string>& ImageDecoder::supportedExtensions() {
	static const std::vector<std::string> exts = {
		// JPEG
		"jpg", "jpeg",
		// PNG
		"png",
		// TIFF
		"tif", "tiff",
		// EXR
		"exr",
		// Camera RAW
		"cr2", "cr3", "nef", "arw", "dng", "raf", "orf", "rw2"
	};
	return exts;
}

// ── Factory ───────────────────────────────────────────────────────────────────

bool ImageDecoder::createImpl(const char* path) {
	ImageFormat fmt = (format != ImageFormat::UNKNOWN) ? format
	                                                   : detectFormat(path);
	switch (fmt) {
		case ImageFormat::JPEG: impl = std::make_unique<JpegDecoderImpl>(); break;
		case ImageFormat::PNG:  impl = std::make_unique<PngDecoderImpl>();  break;
		case ImageFormat::TIFF: impl = std::make_unique<TiffDecoderImpl>(); break;
		case ImageFormat::EXR:  impl = std::make_unique<ExrDecoderImpl>();  break;
		case ImageFormat::RAW:  impl = std::make_unique<RawDecoderImpl>();  break;
		default: return false;
	}
	return true;
}

// ── Streaming API ─────────────────────────────────────────────────────────────

bool ImageDecoder::init(const char* path, int& w, int& h) {
	if (!createImpl(path)) return false;
	if (!impl->open(path, w, h)) return false;
	img_width  = w;
	img_height = h;
	return true;
}

// Returns the number of samples per row: width × channels.
// This is independent of the native bit depth; multiply by bytesPerChannel()
// if you need the byte count for the native representation.
size_t ImageDecoder::rowSize() const {
	return size_t(img_width) * size_t(numChannels());
}

size_t ImageDecoder::readRows(int rows, uint8_t* buf) {
	if (!impl) return 0;
	if (impl->pixelType() == PixelType::UINT8)
		return impl->readRows(rows, buf);

	// Non-UINT8: go through the float path and quantise each sample to [0, 255].
	size_t outRowBytes = rowSize(); // w × ch (8-bit equivalent)
	std::vector<float> fbuf(size_t(rows) * outRowBytes);
	size_t read = impl->readRows(rows, fbuf.data());
	size_t n = read * outRowBytes;
	for (size_t i = 0; i < n; ++i)
		buf[i] = uint8_t(std::min(std::max(fbuf[i], 0.0f), 1.0f) * 255.0f + 0.5f);
	return read;
}

size_t ImageDecoder::readRows(int rows, float* buf) {
	return impl ? impl->readRows(rows, buf) : 0;
}

bool ImageDecoder::finish()  { return impl ? impl->finish()  : false; }
bool ImageDecoder::restart() { return impl ? impl->restart() : false; }

// ── Full-image decode ─────────────────────────────────────────────────────────

bool ImageDecoder::decode(const char* path, uint8_t*& img, int& w, int& h) {
	if (!init(path, w, h)) return false;
	img = new uint8_t[size_t(h) * rowSize()];
	size_t read = readRows(h, img);
	finish();
	return int(read) == h;
}

bool ImageDecoder::decode(const char* path, float*& img, int& w, int& h) {
	if (!init(path, w, h)) return false;
	// Float buffer: one float per channel per pixel.
	size_t nfloats = size_t(w) * size_t(h) * size_t(numChannels());
	img = new float[nfloats];
	size_t read = readRows(h, img);
	finish();
	return int(read) == h;
}

bool ImageDecoder::decode(uint8_t* buf, size_t len, uint8_t*& img, int& w, int& h) {
	// Memory-buffer decode is currently JPEG-only (jpeg_mem_src).
	// PNG, TIFF and EXR all have memory I/O APIs; add them with their backends.
	ImageFormat fmt = (format != ImageFormat::UNKNOWN) ? format : ImageFormat::JPEG;
	if (fmt == ImageFormat::JPEG) {
		impl = std::make_unique<JpegDecoderImpl>();
		auto* ji = static_cast<JpegDecoderImpl*>(impl.get());
		return ji->dec.decode(buf, len, img, w, h);
	}
	return false;
}

// ── Pixel format ──────────────────────────────────────────────────────────────

int       ImageDecoder::numChannels() const { return impl ? impl->numChannels() : 0;                  }
PixelType ImageDecoder::pixelType()   const { return impl ? impl->pixelType()   : PixelType::UINT8;   }

int ImageDecoder::bitsPerChannel() const {
	switch (pixelType()) {
		case PixelType::UINT8:   return  8;
		case PixelType::UINT16:  return 16;
		case PixelType::FLOAT16: return 16;
		case PixelType::FLOAT32: return 32;
	}
	return 0;
}

int ImageDecoder::bytesPerChannel() const {
	switch (pixelType()) {
		case PixelType::UINT8:   return 1;
		case PixelType::UINT16:  return 2;
		case PixelType::FLOAT16: return 2;
		case PixelType::FLOAT32: return 4;
	}
	return 0;
}

// ── ICC colour profile ────────────────────────────────────────────────────────

bool ImageDecoder::hasICCProfile() const {
	return impl && impl->hasICCProfile();
}

const std::vector<uint8_t>& ImageDecoder::getICCProfile() const {
	return impl ? impl->getICCProfile() : empty_profile;
}
