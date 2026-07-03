#include "image_encoder.h"
#include "image_decoder.h"
#include "jpeg_encoder.h"
#include <tiffio.h>

#include <cstring>
#include <algorithm>

// Simple JPEG-backed implementation of ImageEncoderImpl
class JpegImpl : public ImageEncoderImpl {
public:
    JpegImpl() {}
    ~JpegImpl() { finish(); }

    bool open(const char* path, int width, int height, int numChannels) override {
        this->width = width;
        this->height = height;
        this->inChannels = numChannels;

        // Configure color space: use RGB for 3 channels, grayscale for 1.
        if(numChannels == 1) {
            encoder.setColorSpace(JCS_GRAYSCALE, 1);
            encoder.setJpegColorSpace(JCS_GRAYSCALE);
        } else {
            encoder.setColorSpace(JCS_RGB, 3);
            encoder.setJpegColorSpace(JCS_YCbCr);
        }

        // Try to init to file path
        return encoder.init(path, width, height);
    }

    size_t rowSize() const override {
        return size_t(width) * size_t(encoder.getNumComponents());
    }

    size_t writeRows(int rows, const uint8_t* buffer) override {
        if(rows <= 0) return 0;
        int writeCount = 0;
        int expected = encoder.getNumComponents();
        int srcChannels = inChannels;
        size_t srcRowSize = size_t(width) * srcChannels;

        // If source channels match encoder components, write directly
        if(srcChannels == expected) {
            encoder.writeRows(const_cast<uint8_t*>(buffer), rows);
            return rows;
        }

        // Otherwise, convert row-by-row (drop alpha if present, or expand gray->RGB)
        std::vector<uint8_t> tmp(size_t(width) * expected);
        for(int r = 0; r < rows; ++r) {
            const uint8_t* src = buffer + size_t(r) * srcRowSize;
            if(srcChannels == 4 && expected == 3) {
                // drop alpha
                for(int x = 0; x < width; ++x) {
                    tmp[3*x + 0] = src[4*x + 0];
                    tmp[3*x + 1] = src[4*x + 1];
                    tmp[3*x + 2] = src[4*x + 2];
                }
            } else if(srcChannels == 1 && expected == 3) {
                for(int x = 0; x < width; ++x) {
                    uint8_t v = src[x];
                    tmp[3*x + 0] = v;
                    tmp[3*x + 1] = v;
                    tmp[3*x + 2] = v;
                }
            } else if(srcChannels == 3 && expected == 1) {
                for(int x = 0; x < width; ++x) {
                    const uint8_t *p = src + 3*x;
                    tmp[x] = uint8_t((int(p[0]) + int(p[1]) + int(p[2]))/3);
                }
            } else {
                // Unsupported conversion
                return writeCount;
            }
            encoder.writeRows(tmp.data(), 1);
            writeCount++;
        }
        return writeCount;
    }

    size_t writeRows(int rows, const float* buffer) override {
        // Convert float [0,1] to uint8
        std::vector<uint8_t> tmp(size_t(rows) * size_t(width) * encoder.getNumComponents());
        size_t outChannels = encoder.getNumComponents();
        for(int r = 0; r < rows; ++r) {
            const float* src = buffer + size_t(r) * size_t(width) * inChannels;
            for(int x = 0; x < width; ++x) {
                if(inChannels >= 3) {
                    for(size_t c = 0; c < outChannels; ++c) {
                        float v = src[x*inChannels + c];
                        int iv = std::clamp(int(v * 255.0f + 0.5f), 0, 255);
                        tmp[(size_t(r) * width + x) * outChannels + c] = uint8_t(iv);
                    }
                } else if(inChannels == 1 && outChannels == 3) {
                    float v = src[x];
                    int iv = std::clamp(int(v * 255.0f + 0.5f), 0, 255);
                    tmp[(size_t(r) * width + x) * outChannels + 0] = uint8_t(iv);
                    tmp[(size_t(r) * width + x) * outChannels + 1] = uint8_t(iv);
                    tmp[(size_t(r) * width + x) * outChannels + 2] = uint8_t(iv);
                } else {
                    return 0;
                }
            }
        }
        // write rows
        encoder.writeRows(tmp.data(), rows);
        return rows;
    }

    bool finish() override {
        encoder.finish();
        return true;
    }

    int numChannels() const override { return encoder.getNumComponents(); }
    PixelType pixelType() const override { return PixelType::UINT8; }

    bool setICCProfile(const std::vector<uint8_t>& profile) override {
        encoder.setICCProfile(profile);
        return true;
    }

private:
    JpegEncoder encoder;
    int width = 0;
    int height = 0;
    int inChannels = 3;
};

// Simple TIFF-backed implementation of ImageEncoderImpl
class TiffImpl : public ImageEncoderImpl {
public:
    TiffImpl() {}
    ~TiffImpl() { finish(); }

    bool open(const char* path, int width, int height, int numChannels) override {
        this->width = width;
        this->height = height;
        this->channels = numChannels;

        const char *mode = "w";
        tif = TIFFOpen(path, mode);
        if(!tif) return false;

        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, (uint32_t)width);
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, (uint32_t)height);
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, (uint16_t)channels);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, (uint16_t)8);
        TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, (uint32_t)height);
        if(channels == 1)
            TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
        else
            TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

        return true;
    }

    size_t rowSize() const override {
        return size_t(width) * size_t(channels);
    }

    size_t writeRows(int rows, const uint8_t* buffer) override {
        if(!tif) return 0;
        int written = 0;
        size_t rsize = rowSize();
        for(int r = 0; r < rows; ++r) {
            if(current_row >= height) break;
            const uint8_t* ptr = buffer + size_t(r) * rsize;
            if(TIFFWriteScanline(tif, (tdata_t)ptr, current_row, 0) < 0)
                break;
            ++current_row;
            ++written;
        }
        return written;
    }

    size_t writeRows(int rows, const float* /*buffer*/) override {
        // Not implemented for float data
        return 0;
    }

    bool finish() override {
        if(tif) {
            TIFFClose(tif);
            tif = nullptr;
        }
        return true;
    }

    int numChannels() const override { return channels; }
    PixelType pixelType() const override { return PixelType::UINT8; }

    bool setICCProfile(const std::vector<uint8_t>& /*profile*/) override { return false; }

private:
    TIFF* tif = nullptr;
    int width = 0;
    int height = 0;
    int channels = 3;
    int current_row = 0;
};

// ---------------------------------------------------------------------------
// ImageEncoder implementation
// ---------------------------------------------------------------------------

ImageEncoder::ImageEncoder() {}
ImageEncoder::~ImageEncoder() { finish(); }

void ImageEncoder::setFormat(ImageFormat fmt) { format = fmt; }

bool ImageEncoder::encode(const char* path, const uint8_t* img, int width, int height, int numChannels) {
    if(!path || !img) return false;
    // Try to create impl for this path
    if(!createImpl(path)) return false;
    if(!impl) return false;

    if(!impl->open(path, width, height, numChannels)) return false;

    size_t rowBytes = size_t(width) * size_t(numChannels);
    // write all rows at once if backend supports it
    impl->writeRows(height, img);
    impl->finish();
    return true;
}

bool ImageEncoder::encodeToMemory(std::vector<uint8_t>& output, const uint8_t* img, int width, int height, int numChannels) {
    // Only JPEG path implemented for now
    ImageFormat fmt = format;
    if(fmt == ImageFormat::UNKNOWN) {
        // default to JPEG
        fmt = ImageFormat::JPEG;
    }
    if(fmt == ImageFormat::JPEG) {
        JpegEncoder enc;
        if(numChannels == 1) {
            enc.setColorSpace(JCS_GRAYSCALE, 1);
            enc.setJpegColorSpace(JCS_GRAYSCALE);
        } else {
            enc.setColorSpace(JCS_RGB, 3);
            enc.setJpegColorSpace(JCS_YCbCr);
        }
        if(!enc.init(output, width, height)) return false;
        enc.writeRows(const_cast<uint8_t*>(img), height);
        enc.finish();
        return true;
    }
    return false;
}

bool ImageEncoder::init(const char* path, int width, int height, int numChannels) {
    if(!createImpl(path)) return false;
    img_width = width;
    img_height = height;
    return impl->open(path, width, height, numChannels);
}

size_t ImageEncoder::rowSize() const {
    if(!impl) return 0;
    return impl->rowSize();
}

size_t ImageEncoder::writeRows(int rows, const uint8_t* buffer) {
    if(!impl) return 0;
    return impl->writeRows(rows, buffer);
}

size_t ImageEncoder::writeRows(int rows, const float* buffer) {
    if(!impl) return 0;
    return impl->writeRows(rows, buffer);
}

bool ImageEncoder::finish() {
    if(!impl) return true;
    impl->finish();
    impl.reset();
    return true;
}

int ImageEncoder::numChannels() const {
    if(!impl) return 0;
    return impl->numChannels();
}

PixelType ImageEncoder::pixelType() const {
    if(!impl) return PixelType::UINT8;
    return impl->pixelType();
}

int ImageEncoder::bitsPerChannel() const {
    PixelType pt = pixelType();
    switch(pt) {
        case PixelType::UINT8: return 8;
        case PixelType::UINT16: return 16;
        case PixelType::FLOAT16: return 16;
        case PixelType::FLOAT32: return 32;
        default: return 8;
    }
}

int ImageEncoder::bytesPerChannel() const {
    PixelType pt = pixelType();
    switch(pt) {
        case PixelType::UINT8: return 1;
        case PixelType::UINT16: return 2;
        case PixelType::FLOAT16: return 2;
        case PixelType::FLOAT32: return 4;
        default: return 1;
    }
}

bool ImageEncoder::setICCProfile(const std::vector<uint8_t>& profile) {
    if(!impl) return false;
    return impl->setICCProfile(profile);
}

const std::vector<std::string>& ImageEncoder::supportedExtensions() {
    static std::vector<std::string> exts = { "jpg", "jpeg", "png", "tif", "tiff", "exr" };
    return exts;
}

bool ImageEncoder::createImpl(const char* path) {
    ImageFormat fmt = format;
    if(fmt == ImageFormat::UNKNOWN && path) {
        fmt = ImageDecoder::detectFormat(path);
        // If detection returns UNKNOWN, try extension fallback
        if(fmt == ImageFormat::UNKNOWN) {
            std::string s(path);
            auto pos = s.find_last_of('.');
            if(pos != std::string::npos) {
                std::string ext = s.substr(pos+1);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if(ext == "jpg" || ext == "jpeg") fmt = ImageFormat::JPEG;
                else if(ext == "png") fmt = ImageFormat::PNG;
                else if(ext == "tif" || ext == "tiff") fmt = ImageFormat::TIFF;
                else if(ext == "exr") fmt = ImageFormat::EXR;
            }
        }
    }

    // JPEG and TIFF implemented
    if(fmt == ImageFormat::JPEG) {
        impl.reset(new JpegImpl());
        format = ImageFormat::JPEG;
        return true;
    }

    if(fmt == ImageFormat::TIFF) {
        impl.reset(new TiffImpl());
        format = ImageFormat::TIFF;
        return true;
    }

    // other formats not implemented yet
    return false;
}
