// Define the tinyexr implementation in this translation unit only.
#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

#include "exr_decoder.h"

#include <cstring>
#include <cstdlib>
#include <algorithm>

// ── Helpers ───────────────────────────────────────────────────────────────────

// Find the first EXR channel whose name matches one of the given targets.
// Matches exact names ("R") and dotted-suffix names ("diffuse.R", "layer.R").
static int findEXRChannel(const EXRHeader& hdr,
                          std::initializer_list<const char*> targets)
{
	for (const char* target : targets) {
		size_t tlen = strlen(target);
		for (int i = 0; i < hdr.num_channels; ++i) {
			const char* cname = hdr.channels[i].name;
			if (strcmp(cname, target) == 0)
				return i;
			size_t clen = strlen(cname);
			if (clen > tlen + 1 &&
			    cname[clen - tlen - 1] == '.' &&
			    strcmp(cname + clen - tlen, target) == 0)
				return i;
		}
	}
	return -1;
}

// ── ExrDecoderImpl ────────────────────────────────────────────────────────────

bool ExrDecoderImpl::loadImage(const char* path) {
	img_buf.clear();
	channel_map.clear();
	channels    = 0;
	current_row = 0;

	// ── Parse header ─────────────────────────────────────────────────────────
	EXRVersion version;
	if (ParseEXRVersionFromFile(&version, path) != TINYEXR_SUCCESS)
		return false;
	if (version.multipart)
		return false;  // multi-part not yet supported

	EXRHeader header;
	InitEXRHeader(&header);
	const char* err = nullptr;
	if (ParseEXRHeaderFromFile(&header, &version, path, &err) != TINYEXR_SUCCESS) {
		FreeEXRErrorMessage(err);
		return false;
	}

	// Request all channels as FLOAT32 so we get a uniform type back.
	for (int c = 0; c < header.num_channels; ++c)
		header.requested_pixel_types[c] = TINYEXR_PIXELTYPE_FLOAT;

	// ── Load image ────────────────────────────────────────────────────────────
	EXRImage image;
	InitEXRImage(&image);
	if (LoadEXRImageFromFile(&image, &header, path, &err) != TINYEXR_SUCCESS) {
		FreeEXRErrorMessage(err);
		FreeEXRHeader(&header);
		return false;
	}

	width  = image.width;
	height = image.height;

	// ── Channel mapping ───────────────────────────────────────────────────────
	// Try RGB(A), then luminance Y, then all channels in EXR order.
	int r_idx = findEXRChannel(header, {"R", "r"});
	int g_idx = findEXRChannel(header, {"G", "g"});
	int b_idx = findEXRChannel(header, {"B", "b"});
	int a_idx = findEXRChannel(header, {"A", "a"});
	int y_idx = findEXRChannel(header, {"Y", "y"});

	if (r_idx != -1 && g_idx != -1 && b_idx != -1) {
		channel_map.push_back(r_idx);
		channel_map.push_back(g_idx);
		channel_map.push_back(b_idx);
		if (a_idx != -1)
			channel_map.push_back(a_idx);
	} else if (y_idx != -1) {
		channel_map.push_back(y_idx);
	} else {
		// Unknown layout: expose all channels in EXR order.
		channel_map.resize(size_t(image.num_channels));
		for (int c = 0; c < image.num_channels; ++c) channel_map[size_t(c)] = c;
	}
	channels = int(channel_map.size());

	// ── Interleave: SOA (per-channel planes) → AOS (interleaved) ─────────────
	size_t npix = size_t(width) * size_t(height);
	img_buf.resize(npix * size_t(channels));

	for (int c = 0; c < channels; ++c) {
		const float* src = reinterpret_cast<const float*>(
			image.images[size_t(channel_map[size_t(c)])]);
		for (size_t i = 0; i < npix; ++i)
			img_buf[i * size_t(channels) + size_t(c)] = src[i];
	}

	FreeEXRImage(&image);
	FreeEXRHeader(&header);
	return true;
}

bool ExrDecoderImpl::open(const char* path, int& w, int& h) {
	path_copy = path;
	if (!loadImage(path)) return false;
	w = width;
	h = height;
	return true;
}

// Native byte row size (float planes): used by the default float conversion in
// ImageDecoderImpl, but since we override readRows(float*) directly this is
// only relevant for external inspection.
size_t ExrDecoderImpl::rowSize() const {
	return size_t(width) * size_t(channels) * sizeof(float);
}

// This overload is never called in practice: ImageDecoder::readRows(uint8_t*)
// detects non-UINT8 pixel type and routes through the float path instead.
size_t ExrDecoderImpl::readRows(int /*rows*/, uint8_t* /*buf*/) {
	return 0;
}

size_t ExrDecoderImpl::readRows(int rows, float* buf) {
	if (img_buf.empty() || current_row >= height) return 0;
	size_t row_floats = size_t(width) * size_t(channels);
	int read = 0;
	while (read < rows && current_row < height) {
		std::memcpy(buf + size_t(read) * row_floats,
		            img_buf.data() + size_t(current_row) * row_floats,
		            row_floats * sizeof(float));
		++read;
		++current_row;
	}
	return size_t(read);
}

bool ExrDecoderImpl::finish() {
	img_buf.clear();
	img_buf.shrink_to_fit();
	return true;
}

bool ExrDecoderImpl::restart() {
	if (img_buf.empty()) {
		// Buffer was released by finish(); reload.
		if (!loadImage(path_copy.c_str())) return false;
	} else {
		current_row = 0;
	}
	return true;
}

int       ExrDecoderImpl::numChannels() const { return channels;             }
PixelType ExrDecoderImpl::pixelType()   const { return PixelType::FLOAT32;   }
bool      ExrDecoderImpl::hasICCProfile() const { return false; }
const std::vector<uint8_t>& ExrDecoderImpl::getICCProfile() const {
	return ImageDecoder::empty_profile;
}
