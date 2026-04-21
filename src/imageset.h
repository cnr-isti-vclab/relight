#ifndef IMAGESET_H
#define IMAGESET_H

#include "relight_vector.h"
#include "dome.h"
#include "lens.h"
#include "crop.h"
#include "colorprofile.h"
#include <vector>
#include <string>
#include <functional>
#include <string>

#include <lcms2.h>

#include <Eigen/Core>
#include <QStringList>
#include <QPoint>
#include <QSize>

class QJsonObject;
class ImageDecoder;
class QImage;
class QRect;
class Image;
class Project;
class Crop;

class ImageSet {
public:
	QString path;
	QStringList images;
	std::vector<Eigen::Vector3f> lights1;

	int image_width = 0, image_height = 0; //in pixels original size of the image.
	//current crop: left, top is pixel [0, 0];
	int width = 0, height = 0;
	int left = 0, top = 0, right = 0, bottom = 0;
	Crop crop;                  //this is the final crop including rotation (left top right bottom are computed from this before rotation)
	float angle = 0.0f; //
	float cos_a = 1.0f;
	float sin_a = 0.0f;

	float pixel_size = 0;
	Lens lens;

	bool light3d = false;
	float idealLightDistance2 = 0.0f; //squared, used when rescaling intensity

	int current_line = 0;
	std::vector<QPoint> offsets; //align offsets

	ColorProfileMode color_profile_mode = COLOR_PROFILE_LINEAR_RGB;
	std::vector<uint8_t> icc_profile_data;
	cmsHTRANSFORM color_transform = nullptr;               // read path: input ICC → color_profile_mode (working space)
	cmsHTRANSFORM output_color_transform = nullptr;       // write path: working space (uint8) → alternate output (uint8)
	cmsHTRANSFORM output_color_transform_float = nullptr; // write path: working space (float [0,1]) → alternate output (uint8)

	
	ImageSet(const char *path = nullptr);
	~ImageSet();

	//light3d if lights are positional instead of directional.
	//static void parseLP(QString sphere_path, std::vector<Eigen::Vector3f> &lights, std::vector<QString> &filenames, int skip_image = -1);
	static void parseJSON(QString filename,  std::vector<Eigen::Vector3f> &lights);


	//find images in a folder (calls initimages)
	bool initFromFolder(const char *path, int skip_image = -1);
	//load light configuration from dome.
	void initFromDome(Dome &dome);

	//find images in a project, set crop, loads light configuration in dome.
	bool initFromProject(QJsonObject &obj, const QString &filename);

	bool initFromProject(Project &project);

	//open images and starts the decoders
	bool initImages(const char *path, bool force_input_as_linear = false); //path points to the dir of the images.

	//remove not visible images and relative lights.
	void cleanHidden(std::vector<Image> &images);

	void setCrop(int _left, int _top, int _right, int _bottom);
	void setCrop(const Crop &crop);
	void setCrop(Crop &crop, const std::vector<QPointF> &offsets);
	void rotateLights(float angle);



	void setLights(const std::vector<Eigen::Vector3f> &lights, const Dome::LightConfiguration configuration);
	std::vector<Eigen::Vector3f> &lights() { return lights1; }

	// color_profile_mode controls the working colorspace.
	// The input transform converts from input ICC to this space.
	// Changing the mode rebuilds the input transform.
	void setColorProfileMode(ColorProfileMode mode);
	ColorProfileMode getColorProfileMode() const { return color_profile_mode; }

	// Build the input transform (input ICC → color_profile_mode working space).
	// Call after initImages/initFromProject and setColorProfileMode.
	// Identity transforms (e.g. sRGB→sRGB) are detected and result in a null transform (no-op).
	void createColorTransform();

	// Build the output transform (working space → target). If target is the same as
	// the working space, both transforms are null (no-op).
	// If target is not specified, defaults to color_profile_mode.
	void createOutputColorTransform(ColorProfileMode target);
	void createOutputColorTransform();

	// Apply the output colorspace transform (working space → target).
	// Call this on pixel data just before writing to an output file.
	void applyOutputColorTransform(uint8_t *data, size_t pixel_count);

	// Apply the output colorspace transform using float [0,1] input for higher precision.
	// in01 must be normalized to [0.0, 1.0]. Writes result as uint8 to out.
	void applyOutputColorTransformFloat(float *in01, uint8_t *out, size_t pixel_count);

	// Returns the ICC profile data that matches the current working colorspace.
	// Embed this in output JPEG files so readers can interpret the pixels correctly.
	const std::vector<uint8_t> getOutputICCProfile() const;
	// Returns the ICC profile data for a specific colorspace.
	const std::vector<uint8_t> getOutputICCProfile(ColorProfileMode mode) const;

	size_t size() { return size_t(images.size()); }

	bool hasICCProfile() const { return !icc_profile_data.empty(); }
	const std::vector<uint8_t> &getICCProfile() const { return icc_profile_data; }

	QImage maxImage(std::function<bool(std::string stage, int percent)> *callback = nullptr); 
	QSize imageSize() { return QSize(image_width, image_height); }

	void setCallback(std::function<bool(QString stage, int percent)> *_callback = nullptr) { callback = _callback; }
	//call AFTER initImages and BEFORE breadline, decode or sample.

	void decode(size_t img, unsigned char *buffer);
	void readLine(PixelArray &line);
	uint32_t sample(PixelArray &sample, uint32_t ndimensions, std::function<void(Pixel &, Pixel &)> resampler, uint32_t samplingrate);
	void restart();
	void skipToTop();

	Eigen::Vector3f relativeLight(const Eigen::Vector3f &light, int x, int y);

protected:
	std::function<bool(QString stage, int percent)> *callback;
	std::vector<ImageDecoder *> decoders;


private:
	void compensateVignetting(PixelArray &pixels);

	void compensateIntensity(PixelArray &pixels);
	void applyColorTransform(uint8_t *data, size_t pixel_count);
};

#endif // IMAGESET_H
