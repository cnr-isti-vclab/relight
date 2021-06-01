#ifndef RTI_H
#define RTI_H

#include "material.h"
#include "vector.h"

#include <vector>
#include <string>
#include <map>

/*
 * RGB implies each plane is processed independently (but might be saved in jpeg ycc)
 * LRGB implies first 3 planes are rgb, the rest are luminance planes
 * YCC same as RGB but might use different number of planes for Y
 * MRGB mixed: each plane coeff weights an RGB basis
 */

class ImageSet;
class QString;

class Rti {
public:

	enum Type       { PTM = 0, HSH = 1, RBF = 2, BILINEAR = 3, DMD = 4, SH = 5, H = 6 };
	enum ColorSpace { RGB = 0, LRGB = 1, YCC = 2, MRGB = 3, MYCC = 4 };
	enum Format     { JSON = 0, BRTI = 1 };
	enum ImgFormat  { RAW = 0, JPEG = 1, PNG = 2 }; //add other formats?

	std::map<std::string, std::string> exif;

	Type type = RBF;
	ColorSpace colorspace = MRGB;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t nplanes = 9;
	uint32_t yccplanes[3] = {0, 0, 0}; //for ycc format (nplanes- yplanes)/2 = cplanes
	float sigma = 0.125; //rbf interpolation parameter default for ~100 images
	float regularization = 0.1f; //bilinear regularization coeff.
	bool chromasubsampling = false;
	bool gammaFix = false;

	std::vector<std::vector<uint8_t>> planes;

	//PTM light_coeff * (plane_coeff - bias) * scale //in 0-255
	//HSH light_coeff * (plane_coeff * scale + bias) // in 0-1
	//PCA (plane_coeff - 127)/255)*scale + bias      // in 0-255

	//TODO internally we should use only one system
	//since scale and bias are float we use HSH [0-1] system!
	std::vector<float> scale;
	std::vector<float> bias;
	std::vector<float> range; // material scale (no need for bias, it is zero centered)

	Material material;
	std::vector<float> basis; //for each material, first mean then each plane
								//rbf ordered following lights
								//bilinear as a matrix x + y*width
	std::vector<Vector3f> lights; //for rgb lx, ly, lz
	uint32_t resolution = 8; // for bilinear
	uint32_t ndimensions = 0; //for pca stuff

	size_t filesize = 0; //total filesize for statistics
	size_t headersize = 0;
	std::vector<size_t> planesize;
	std::string error; //for error reporting

	Rti() {}
	bool load(const char *filename, bool loadPlanes = true);
	bool loadData(const char *folder);
//	bool save(const char *filename, Format format = JSON, ImgFormat img_format = JPEG, int quality = 90);
    void render(float lx, float ly, uint8_t *img, int stride = 3, uint32_t renderplanes = 0);
	void clip(int left, int bottom, int right, int top); //right and top pixel excluded
	Rti clipped(int left, int bottom, int right, int top);
	static double evaluateError(ImageSet &imageset, Rti &rti, QString output, int reference = -1);


	std::vector<float> lightWeights   (float lx, float ly);
	std::vector<float> lightWeightsPtm(float lx, float ly);
	std::vector<float> lightWeightsHsh(float lx, float ly);
	std::vector<float> lightWeightsSh (float lx, float ly);
	std::vector<float> lightWeightsH  (float lx, float ly);
	std::vector<float> lightWeightsDmd(float lx, float ly);
	std::vector<float> lightWeightsRbf(float lx, float ly);
	std::vector<float> lightWeightsBilinear(float lx, float ly);

protected:
	std::vector<float> rbfWeights(float lx, float ly);

	//find offset of a basis element in the basis array
	size_t basePixelOffset(size_t m, size_t p, size_t x, size_t y, size_t k);
};

#endif // RTI_H
