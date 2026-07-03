#ifndef IMAGE_FORMAT_H
#define IMAGE_FORMAT_H

#include <cstdint>

// Shared enums for image I/O (decoders and encoders).
// Keep these definitions in a single header to avoid duplication.

// Supported image formats
enum class ImageFormat {
    UNKNOWN,
    JPEG,
    PNG,
    TIFF,
    EXR,
    RAW
};

// Pixel element type (valid after init() or decode()/encode()).
enum class PixelType {
    UINT8,   //  8 bits per channel
    UINT16,  // 16 bits per channel
    FLOAT16, // half float
    FLOAT32  // 32-bit float
};

#endif // IMAGE_FORMAT_H
