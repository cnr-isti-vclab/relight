#ifndef IMAGE_ENCODER_H
#define IMAGE_ENCODER_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// Shared image format and pixel type enums
#include "image_format.h"

// ──────────────────────────────────────────────────────────────────────────────
// Abstract per-format backend for encoders.
// Concrete implementations (JpegEncoder wrapper, PNG/TIFF/EXR backends) live
// in their respective translation units and are not exposed publicly.
// ──────────────────────────────────────────────────────────────────────────────
struct ImageEncoderImpl {
    virtual ~ImageEncoderImpl() = default;

    // Open a destination for writing and initialise encoder state.
    virtual bool open(const char* path, int width, int height, int numChannels) = 0;

    // Bytes per complete scanline: width * numChannels * bytesPerChannel().
    virtual size_t rowSize() const = 0;

    // Write up to `rows` scanlines from `buffer` (uint8_t raw bytes).
    // Returns the number of rows actually written.
    virtual size_t writeRows(int rows, const uint8_t* buffer) = 0;

    // Float variant: normalised floats in [0,1] for integer formats.
    virtual size_t writeRows(int rows, const float* buffer);

    // Finalise and close output. Should be idempotent.
    virtual bool finish() = 0;

    // Number of channels the encoder expects (1, 3 or 4).
    virtual int       numChannels() const = 0;
    virtual PixelType pixelType()   const = 0;

    // Optional: embed ICC profile. Returns false when unsupported.
    virtual bool setICCProfile(const std::vector<uint8_t>& profile) { return false; }
};

// ──────────────────────────────────────────────────────────────────────────────
// ImageEncoder — public API, format-agnostic. Mirrors the design of
// ImageDecoder while providing basic full-image and streaming encode APIs.
// ──────────────────────────────────────────────────────────────────────────────
class ImageEncoder {
public:
    ImageEncoder();
    ~ImageEncoder();

    ImageEncoder(const ImageEncoder&) = delete;
    void operator=(const ImageEncoder&) = delete;

    // Override auto-detection. Call before init() or encode().
    void        setFormat(ImageFormat fmt);
    ImageFormat getFormat() const { return format; }

    // Full-image encode (uint8_t buffer). Returns true on success.
    // `img` is expected to be interleaved RGB/RGBA/Gray, row-major.
    bool encode(const char* path, const uint8_t* img, int width, int height, int numChannels);
    bool encode(const std::string& path, const std::vector<uint8_t>& img, int width, int height, int numChannels)
        { return encode(path.c_str(), img.data(), width, height, numChannels); }

    // In-memory output (some backends may support writing into a vector).
    bool encodeToMemory(std::vector<uint8_t>& output, const uint8_t* img, int width, int height, int numChannels);

    // ── Streaming row-by-row API ──────────────────────────────────────────────
    bool   init(const char* path, int width, int height, int numChannels);
    size_t rowSize() const;
    size_t writeRows(int rows, const uint8_t* buffer);
    size_t writeRows(int rows, const float* buffer);
    bool   finish();

    // ── Pixel format / capabilities ───────────────────────────────────────────
    int       numChannels()     const;   // 1, 3, or 4
    PixelType pixelType()       const;
    int       bitsPerChannel()  const;   // 8, 16, or 32
    int       bytesPerChannel() const;   // 1, 2, or 4

    // Embed ICC profile if supported by current backend.
    bool setICCProfile(const std::vector<uint8_t>& profile);

    // Returns all lowercase file extensions this encoder can write.
    static const std::vector<std::string>& supportedExtensions();

private:
    bool createImpl(const char* path);

    ImageFormat format = ImageFormat::UNKNOWN;
    int img_width  = 0;
    int img_height = 0;
    std::unique_ptr<ImageEncoderImpl> impl;
};

#endif // IMAGE_ENCODER_H
