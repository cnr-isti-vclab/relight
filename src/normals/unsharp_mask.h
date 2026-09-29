#ifndef UNSHARP_MASK_H
#define UNSHARP_MASK_H

#include <Eigen/Core>
#include <QPixmap>
#include <QString>

#include <string>
#include <vector>

enum class NormalRenderScheme {
	OpenGL,
	DirectX,
	Grayscale,
	Specular
};

enum class HeightColorScheme {
	Grayscale,
	Viridis,
	Plasma,
	Inferno,
	Magma,
	Cividis,
	Turbo
};

enum class SharpenInputType {
	Image,
	Normals,
	Heightmap
};

// Convenient container for source data and interactive settings.
// Only the data vector selected by type should be populated.
struct SharpenData {
	SharpenInputType type = SharpenInputType::Image;
	std::vector<Eigen::Vector3f> image;
	std::vector<Eigen::Vector3f> normals;
	std::vector<float> heightmap;

	unsigned int width = 0;
	unsigned int height = 0;
	float sigma = 10.0f;

	// Range of the decoded source heightmap; preview limits are based on the
	// currently processed heightmap.
	float min = 0.0f;
	float max = 1.0f;

	NormalRenderScheme normalColorScheme = NormalRenderScheme::OpenGL;
	HeightColorScheme heightColorScheme = HeightColorScheme::Viridis;

	// Decode color samples as floats, retaining the decoder's numeric range.
	void loadImage(const std::string &path);
	// Normal maps with RGB channels are decoded from [0,1] to signed vectors
	// for integer formats, and kept in their native signed range for float data.
	void loadNormalmap(const std::string &path);
	// Grayscale inputs use their sole channel; color inputs are converted to
	// luminance. The original data range is recorded in min/max.
	void loadHeightmap(const std::string &path);
};

// Return the high-frequency residual (input minus its Gaussian-smoothed form).
// Image Vector3f inputs are smoothed independently per component. Normal input
// is converted to x/z and y/z slopes; the result is the XY high-frequency tilt
// residual (z = 0), not a normal map, and is not renormalized or integrated.
std::vector<Eigen::Vector3f> unsharpMaskImage(
		const std::vector<Eigen::Vector3f> &image,
		unsigned int width, unsigned int height, float sigma);

std::vector<Eigen::Vector2f> unsharpMaskNormals(
		const std::vector<Eigen::Vector3f> &normals,
		unsigned int width, unsigned int height, float sigma);

std::vector<float> unsharpMaskHeightmap(
		const std::vector<float> &heightmap,
		unsigned int width, unsigned int height, float sigma);

// Convert interleaved normalized RGB pixels in the 0..1 range to a display pixmap.
QPixmap renderImage(const std::vector<Eigen::Vector3f> &image,
		unsigned int width, unsigned int height);

// Render slope residuals as an OpenGL/DirectX normal map, grayscale facing map,
// or a simple specular-lit view.
QPixmap renderNormal(const std::vector<Eigen::Vector2f> &normalDetail,
		unsigned int width, unsigned int height, NormalRenderScheme scheme);

// Map the selected value range to a false-color gradient.
QPixmap renderHeight(const std::vector<float> &heightmap,
		unsigned int width, unsigned int height,
	float min_value, float max_value, HeightColorScheme scheme);

#endif // UNSHARP_MASK_H
