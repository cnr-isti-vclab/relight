#include "unsharp_mask.h"

#include "fast_gaussian_blur.h"
#include "../image_decoder.h"

#include <QImage>
#include <QString>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
float luminance(const float *pixel, int channels) {
	if(channels <= 2)
		return pixel[0];
	return 0.2126f*pixel[0] + 0.7152f*pixel[1] + 0.0722f*pixel[2];
}

void validateDimensions(size_t count, unsigned int width, unsigned int height) {
	if(width == 0 || height == 0 || count != size_t(width) * height)
		throw std::invalid_argument("Unsharp mask input size must match non-zero image dimensions");
}


} // End of anonymous namespace
void SharpenData::loadImage(const std::string &path) {
	ImageDecoder decoder;
	float *decodedPixels = nullptr;
	int decodedWidth = 0;
	int decodedHeight = 0;
	const bool decoded = decoder.decode(path.c_str(), decodedPixels, decodedWidth, decodedHeight);
	std::unique_ptr<float[]> pixels(decodedPixels);
	const int channels = decoder.numChannels();
	if(!decoded) {
		throw QString("The image decoder could not read the file or does not support its format.");
	}
	if(decodedWidth <= 0 || decodedHeight <= 0 || (channels != 1 && channels != 3)) {
		throw QString("The decoded image has invalid dimensions or an unsupported channel count.");
	}
	const size_t count = size_t(decodedWidth) * size_t(decodedHeight);
	std::vector<Eigen::Vector3f> loaded(count);
	for(size_t i = 0; i < count; ++i) {
		const float *pixel = pixels.get() + i*size_t(channels);
		if(channels == 1)
			loaded[i] = Eigen::Vector3f::Constant(pixel[0]);
		else
			loaded[i] = Eigen::Vector3f(pixel[0], pixel[1], pixel[2]);
	}

	image = std::move(loaded);
	normals.clear();
	heightmap.clear();
	width = static_cast<unsigned int>(decodedWidth);
	height = static_cast<unsigned int>(decodedHeight);
	type = SharpenInputType::Image;
}

void SharpenData::loadNormalmap(const std::string &path) {
	ImageDecoder decoder;
	float *decodedPixels = nullptr;
	int decodedWidth = 0;
	int decodedHeight = 0;
	const bool decoded = decoder.decode(path.c_str(), decodedPixels, decodedWidth, decodedHeight);
	std::unique_ptr<float[]> pixels(decodedPixels);
	const int channels = decoder.numChannels();
	if(!decoded) {
		throw QString("The image decoder could not read the file or does not support its format.");
	}
	if(decodedWidth <= 0 || decodedHeight <= 0 || channels != 3) {
		throw QString("A normal map must have valid dimensions and three color channels.");
	}

	const size_t count = size_t(decodedWidth) * size_t(decodedHeight);
	std::vector<Eigen::Vector3f> loaded(count);
	const bool encoded = decoder.pixelType() == PixelType::UINT8 || decoder.pixelType() == PixelType::UINT16;
	for(size_t i = 0; i < count; ++i) {
		const float *pixel = pixels.get() + i*size_t(channels);
		Eigen::Vector3f normal(pixel[0], pixel[1], pixel[2]);
		if(encoded)
			normal = normal * 2.0f - Eigen::Vector3f::Ones();
		if(!normal.allFinite() || normal.squaredNorm() < 1e-12f)
			normal = Eigen::Vector3f(0.0f, 0.0f, 1.0f);
		else
			normal.normalize();
		loaded[i] = normal;
	}

	normals = std::move(loaded);
	image.clear();
	heightmap.clear();
	width = static_cast<unsigned int>(decodedWidth);
	height = static_cast<unsigned int>(decodedHeight);
	type = SharpenInputType::Normals;
}

void SharpenData::loadHeightmap(const std::string &path) {
	ImageDecoder decoder;
	float *decodedPixels = nullptr;
	int decodedWidth = 0;
	int decodedHeight = 0;
	const bool decoded = decoder.decode(path.c_str(), decodedPixels, decodedWidth, decodedHeight);
	std::unique_ptr<float[]> pixels(decodedPixels);
	const int channels = decoder.numChannels();
	if(!decoded) {
		throw QString("The image decoder could not read the file or does not support its format.");
	}
	if(decodedWidth <= 0 || decodedHeight <= 0 || channels != 1) {
		throw QString("A heightmap must have valid dimensions and one channel.");
	}
	const size_t count = size_t(decodedWidth) * size_t(decodedHeight);
	std::vector<float> loaded(count);
	float loadedMin = std::numeric_limits<float>::infinity();
	float loadedMax = -std::numeric_limits<float>::infinity();
	for(size_t i = 0; i < count; ++i) {
		const float value = luminance(pixels.get() + i*size_t(channels), channels);
		loaded[i] = value;
		if(std::isfinite(value)) {
			loadedMin = std::min(loadedMin, value);
			loadedMax = std::max(loadedMax, value);
		}
	}
	if(!std::isfinite(loadedMin) || !std::isfinite(loadedMax)) {
		loadedMin = 0.0f;
        loadedMax = 1.0f;
    }

	heightmap = std::move(loaded);
	image.clear();
	normals.clear();
	width = static_cast<unsigned int>(decodedWidth);
	height = static_cast<unsigned int>(decodedHeight);
	min = loadedMin;
	max = loadedMax;
	type = SharpenInputType::Heightmap;
}

namespace {
std::vector<Eigen::Vector3f> unsharpMaskVector3(
		const std::vector<Eigen::Vector3f> &input,
		unsigned int width, unsigned int height, float sigma) {
	validateDimensions(input.size(), width, height);
	if(sigma <= 0.0f)
		return std::vector<Eigen::Vector3f>(input.size(), Eigen::Vector3f::Zero());

	std::vector<Eigen::Vector3f> result(input.size());
	for(int component = 0; component < 3; ++component) {
		std::vector<float> blurred(input.size());
		for(size_t i = 0; i < input.size(); ++i)
			blurred[i] = input[i][component];

		fast_gaussian_blur(blurred, width, height, sigma);
		for(size_t i = 0; i < input.size(); ++i)
			result[i][component] = input[i][component] - blurred[i];
	}
	return result;
}

uint8_t toByte(float value) {
	if(!std::isfinite(value))
		return 0;
	return static_cast<uint8_t>(std::lround(std::clamp(value, 0.0f, 255.0f)));
}

QPixmap pixmapFromRgb(QImage &image) {
	return QPixmap::fromImage(image);
}

using Color = std::array<float, 3>;

Color paletteColor(HeightColorScheme scheme, float t) {
	static const std::array<Color, 5> viridis = {{{68, 1, 84}, {59, 82, 139}, {33, 145, 140}, {94, 201, 98}, {253, 231, 37}}};
	static const std::array<Color, 5> plasma  = {{{13, 8, 135}, {126, 3, 168}, {204, 71, 120}, {248, 149, 64}, {240, 249, 33}}};
	static const std::array<Color, 5> inferno = {{{0, 0, 4}, {87, 15, 109}, {187, 55, 84}, {249, 142, 9}, {252, 255, 164}}};
	static const std::array<Color, 5> magma   = {{{0, 0, 4}, {81, 18, 124}, {183, 55, 121}, {252, 137, 97}, {252, 253, 191}}};
	static const std::array<Color, 5> cividis = {{{0, 34, 78}, {63, 76, 107}, {124, 123, 120}, {189, 174, 109}, {254, 232, 56}}};
	static const std::array<Color, 5> turbo   = {{{48, 18, 59}, {50, 104, 220}, {35, 210, 160}, {250, 220, 50}, {122, 4, 3}}};

	if(scheme == HeightColorScheme::Grayscale)
		return {255.0f*t, 255.0f*t, 255.0f*t};

	const Color *colors = viridis.data();
	switch(scheme) {
	case HeightColorScheme::Viridis: colors = viridis.data(); break;
	case HeightColorScheme::Plasma: colors = plasma.data(); break;
	case HeightColorScheme::Inferno: colors = inferno.data(); break;
	case HeightColorScheme::Magma: colors = magma.data(); break;
	case HeightColorScheme::Cividis: colors = cividis.data(); break;
	case HeightColorScheme::Turbo: colors = turbo.data(); break;
	case HeightColorScheme::Grayscale: break;
	}
	const float scaled = t * 4.0f;
	const int index = std::min(3, static_cast<int>(scaled));
	const float fraction = scaled - index;
	Color color;
	for(int c = 0; c < 3; ++c)
		color[c] = colors[index][c] * (1.0f - fraction) + colors[index + 1][c] * fraction;
	return color;
}

}

std::vector<Eigen::Vector3f> unsharpMaskImage(
		const std::vector<Eigen::Vector3f> &image,
		unsigned int width, unsigned int height, float sigma) {
	return unsharpMaskVector3(image, width, height, sigma);
}

std::vector<Eigen::Vector2f> unsharpMaskNormals(
		const std::vector<Eigen::Vector3f> &normals,
		unsigned int width, unsigned int height, float sigma) {
	validateDimensions(normals.size(), width, height);
	if(sigma <= 0.0f)
		return std::vector<Eigen::Vector2f>(normals.size(), Eigen::Vector2f::Zero());

	// A normal's x/z and y/z components describe the local surface slopes.
	// Filter those directional slopes independently, rather than filtering the
	// encoded normal-map color channels.
	std::vector<float> slopeX(normals.size());
	std::vector<float> slopeY(normals.size());
	for(size_t i = 0; i < normals.size(); ++i) {
		const float z = normals[i].z();
		const float safeZ = std::abs(z) < 1e-6f ? std::copysign(1e-6f, z == 0.0f ? 1.0f : z) : z;
		slopeX[i] = normals[i].x() / safeZ;
		slopeY[i] = normals[i].y() / safeZ;
	}

	std::vector<float> blurredX = slopeX;
	std::vector<float> blurredY = slopeY;
	fast_gaussian_blur(blurredX, width, height, sigma);
	fast_gaussian_blur(blurredY, width, height, sigma);

	// Return the high-frequency tilt residual as an XY vector field. This is
	// detail data, not a normal map: do not renormalize or integrate it.
	std::vector<Eigen::Vector2f> result(normals.size());
	for(size_t i = 0; i < normals.size(); ++i)
		result[i] = Eigen::Vector2f(slopeX[i] - blurredX[i], slopeY[i] - blurredY[i]);
	return result;
}

std::vector<float> unsharpMaskHeightmap(
		const std::vector<float> &heightmap,
		unsigned int width, unsigned int height, float sigma) {
	validateDimensions(heightmap.size(), width, height);
	if(sigma <= 0.0f)
		return std::vector<float>(heightmap.size(), 0.0f);

	std::vector<float> result = heightmap;
	fast_gaussian_blur(result, width, height, sigma);
	for(size_t i = 0; i < result.size(); ++i)
		result[i] = heightmap[i] - result[i];
	return result;
}

QPixmap renderImage(const std::vector<Eigen::Vector3f> &image,
		unsigned int width, unsigned int height) {
	validateDimensions(image.size(), width, height);
	QImage rgb(width, height, QImage::Format_RGB888);
	for(unsigned int y = 0; y < height; ++y) {
		uint8_t *row = rgb.scanLine(y);
		for(unsigned int x = 0; x < width; ++x) {
			const Eigen::Vector3f &pixel = image[x + y*width];
			for(int c = 0; c < 3; ++c)
				row[3*x + c] = toByte(pixel[c] * 255.0f);
		}
	}
	return pixmapFromRgb(rgb);
}

QPixmap renderNormal(const std::vector<Eigen::Vector2f> &normalDetail,
		unsigned int width, unsigned int height, NormalRenderScheme scheme) {
	validateDimensions(normalDetail.size(), width, height);
	QImage rgb(width, height, QImage::Format_RGB888);
	const Eigen::Vector3f light = Eigen::Vector3f(0.3f, -0.4f, 0.8660254f).normalized();
	for(unsigned int y = 0; y < height; ++y) {
		uint8_t *row = rgb.scanLine(y);
		for(unsigned int x = 0; x < width; ++x) {
			const Eigen::Vector2f &slope = normalDetail[x + y*width];
			Eigen::Vector3f normal(-slope.x(), -slope.y(), 1.0f);
			if(!normal.allFinite())
				normal = Eigen::Vector3f(0.0f, 0.0f, 1.0f);
			normal.normalize();
			if(scheme == NormalRenderScheme::DirectX)
				normal.y() = -normal.y();

			float color[3];
			switch(scheme) {
			case NormalRenderScheme::OpenGL:
			case NormalRenderScheme::DirectX:
				for(int c = 0; c < 3; ++c)
					color[c] = (normal[c] + 1.0f) * 127.5f;
				break;
			case NormalRenderScheme::Grayscale:
				color[0] = color[1] = color[2] = (normal.z() + 1.0f) * 127.5f;
				break;
			case NormalRenderScheme::Specular: {
				const float diffuse = std::max(0.0f, normal.dot(light));
				const Eigen::Vector3f halfVector = (light + Eigen::Vector3f(0.0f, 0.0f, 1.0f)).normalized();
				const float specular = std::pow(std::max(0.0f, normal.dot(halfVector)), 32.0f);
				const float intensity = 24.0f + 100.0f*diffuse + 131.0f*specular;
				color[0] = intensity;
				color[1] = intensity;
				color[2] = intensity;
				break;
			}
			}
			for(int c = 0; c < 3; ++c)
				row[3*x + c] = toByte(color[c]);
		}
	}
	return pixmapFromRgb(rgb);
}

QPixmap renderHeight(const std::vector<float> &heightmap,
		unsigned int width, unsigned int height,
		float min_value, float max_value, HeightColorScheme scheme) {
	validateDimensions(heightmap.size(), width, height);
	if(min_value > max_value)
		std::swap(min_value, max_value);
	const float range = max_value - min_value;

	QImage rgb(width, height, QImage::Format_RGB888);
	for(unsigned int y = 0; y < height; ++y) {
		uint8_t *row = rgb.scanLine(y);
		for(unsigned int x = 0; x < width; ++x) {
			const float value = heightmap[x + y*width];
			const float t = !std::isfinite(value) ? 0.0f :
				(range > std::numeric_limits<float>::epsilon() ?
				 std::clamp((value - min_value) / range, 0.0f, 1.0f) : 0.5f);
			const Color color = paletteColor(scheme, t);
			for(int c = 0; c < 3; ++c)
				row[3*x + c] = toByte(color[c]);
		}
	}
	return pixmapFromRgb(rgb);
}

