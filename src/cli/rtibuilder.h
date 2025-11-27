#ifndef RTIBUILDER_H
#define RTIBUILDER_H

#include <QString>

#include "../src/rti.h"
#include "../src/imageset.h"
#include "../src/dome.h"
#include "../src/material.h"
#include "../src/relight_vector.h"

#include <Eigen/Core>

#include <functional>
class QDir;

//store pair light, coefficients for each resampled light direction.
typedef std::vector<std::vector<std::pair<int, float>>> Resamplemap;

// ICC color profile handling mode
enum ColorProfileMode {
	COLOR_PROFILE_PRESERVE,  // Pass through input ICC profile
	COLOR_PROFILE_SRGB       // Convert to sRGB (requires LittleCMS2)
};

class RtiBuilder: public Rti {
public:
	ImageSet imageset;
	//float pixelSize = 0;
	uint32_t samplingram = 500;
	uint32_t nsamples = 1<<16; //TODO change to rate
	float rangescale = 1.5;
	int skip_image = -1;

	float rangeQuantile = 0.995f; //quantile to use for histogram-based range compression (default 99.5%)
	bool commonMinMax = false; //use the same min and max for RGB planes only used for exporting .rti and .ptm
	bool histogram_fix = false;
	bool savenormals = false;
	bool savemeans = false;
	bool savemedians = false;
	int crop[4] = { 0, 0, 0, 0 }; //left, top, width, height
	size_t nworkers = 0; //autodetect optimal number
	ColorProfileMode colorProfileMode = COLOR_PROFILE_PRESERVE;

	std::function<bool(QString stage, int percent)> *callback = nullptr;

	RtiBuilder();
	~RtiBuilder();
	bool setupFromFolder(const std::string &folder);
	bool setupFromProject(const std::string &filename);
	bool init(std::function<bool(QString stage, int percent)> *_callback = nullptr);

	size_t save(const std::string &output, int quality = 95);
	size_t savePTM(const std::string &output);
	size_t saveUniversal(const std::string &output);
	bool saveJSON(QDir &dir, int quality);

	void processLine(PixelArray &sample, PixelArray &resample, std::vector<std::vector<uint8_t>> &line,
					 std::vector<uchar> &normal, std::vector<uchar> &mean, std::vector<uchar> &median);

protected:
	MaterialBuilder materialbuilder;

	//for each resample pos get coeffs from the origina lights.
	Resamplemap resamplemap;

	//grid of resamplemaps to be interpolated.
	int resample_width = 9, resample_height = 9;
	std::vector<Resamplemap> resamplemaps;  //for per pixel direction light interpolation
	std::vector<MaterialBuilder> materialbuilders;

	//TODO this should go inimageset!
	//compute the 3d lights relative to the pixel x, y
	std::vector<Eigen::Vector3f> relativeNormalizedLights(int x, int y);

	void resamplePixel(Pixel &sample, Pixel &pixel);

	void buildResampleMap(std::vector<Eigen::Vector3f> &lights, std::vector<std::vector<std::pair<int, float> > > &remap);
	void buildResampleMaps();
	void remapPixel(Pixel &sample, Pixel &pixel, Resamplemap &resamplemap, float weight);



	MaterialBuilder pickBase(PixelArray &sample, std::vector<Eigen::Vector3f> &lights);
	//use for 3d lights
	void pickBases(PixelArray &sample);
	//PTM or HSH + bad light sampling could overestimate, here histogram is renormalized.
	//TODO: check for underestimate, expose parameter.
	void normalizeHistogram(PixelArray &sample, double percentile = 0.95);

	void minmaxMaterial(PixelArray &sample);
	void finalizeMaterial();



	void estimateError(PixelArray &sample, std::vector<float> &weights);
	void getPixelMaterial(PixelArray &pixels, std::vector<size_t> &indices);
	void getPixelBestMaterial(PixelArray &pixels, std::vector<size_t> &indices);

	MaterialBuilder pickBasePCA(PixelArray &sample);
	MaterialBuilder pickBasePTM(std::vector<Eigen::Vector3f> &lights);
	MaterialBuilder pickBaseHSH(std::vector<Eigen::Vector3f> &lights, Type base = HSH);

	Eigen::Vector3f getNormalThreeLights(std::vector<float> &pri); //use 3 virtual lights at 45 degs.

//DEBUG
	
	//void saveLightPixel(Color3f *p, int side, const QString &file);
	//void savePixel(Color3f *p, int side, const QString &file);
	void debugMaterials();

	std::vector<float> toPrincipal(Pixel &pixel, MaterialBuilder &materialbuilder);
	std::vector<float> toPrincipal(Pixel &pixel);

};


#endif // RTIBUILDER_H
