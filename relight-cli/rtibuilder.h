#ifndef RTIBUILDER_H
#define RTIBUILDER_H

#include <QString>

#include "../src/rti.h"
#include "../src/imageset.h"
#include "../src/material.h"

#include <Eigen/Core>

#include <functional>
class QDir;

//store pair light, coefficients for each resampled light direction.
typedef std::vector<std::vector<std::pair<int, float>>> Resamplemap;

class RtiBuilder: public Rti {
public:
	ImageSet imageset;
	float pixelSize = 0;
	uint32_t samplingram = 500;
	uint32_t nsamples = 1<<16; //TODO change to rate
	float rangescale = 1.5;
	int skip_image = -1;
	//TODO: might want to use an euristic to get the best compromise to miniminze jpeg compression artifacts.
	float rangecompress = 0.5f; //betwee 0 and 1, where 0 is maximally compressed
	bool commonMinMax = false; //use the same min and max for RGB planes only used for exporting .rti and .ptm
	bool savenormals = false;
	bool savemeans = false;
	bool savemedians = false;
	int crop[4] = { 0, 0, 0, 0 }; //left, top, width, height
	size_t nworkers = 8;

	std::function<bool(std::string stage, int percent)> *callback = nullptr;

	RtiBuilder();
	~RtiBuilder();
	bool initFromFolder(const std::string &folder, std::function<bool(std::string stage, int percent)> *_callback = nullptr);
	bool initFromProject(const std::string &filename, std::function<bool(std::string stage, int percent)> *_callback = nullptr);
	bool init(std::function<bool(std::string stage, int percent)> *_callback = nullptr);

	size_t save(const std::string &output, int quality = 95);
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
	std::vector<Vector3f> relativeLights(int x, int y);

	void resamplePixel(Pixel &sample, Pixel &pixel);

	void buildResampleMap(std::vector<Vector3f> &lights, std::vector<std::vector<std::pair<int, float> > > &remap);
	void buildResampleMaps();
	void remapPixel(Pixel &sample, Pixel &pixel, Resamplemap &resamplemap, float weight);



	MaterialBuilder pickBase(PixelArray &sample, std::vector<Vector3f> &lights);
	//use for 3d lights
	void pickBases(PixelArray &sample);
	void minmaxMaterial(PixelArray &sample);
	void finalizeMaterial();



	void estimateError(PixelArray &sample, std::vector<float> &weights);
	void getPixelMaterial(PixelArray &pixels, std::vector<size_t> &indices);
	void getPixelBestMaterial(PixelArray &pixels, std::vector<size_t> &indices);

	MaterialBuilder pickBasePCA(PixelArray &sample);
	MaterialBuilder pickBasePTM(std::vector<Vector3f> &lights);
	MaterialBuilder pickBaseHSH(std::vector<Vector3f> &lights, Type base = HSH);

	Vector3f getNormalThreeLights(std::vector<float> &pri); //use 3 virtual lights at 45 degs.

//DEBUG
	
	//void saveLightPixel(Color3f *p, int side, const QString &file);
	//void savePixel(Color3f *p, int side, const QString &file);
	void debugMaterials();

	std::vector<float> toPrincipal(Pixel &pixel, MaterialBuilder &materialbuilder);
	std::vector<float> toPrincipal(Pixel &pixel);

};


#endif // RTIBUILDER_H
