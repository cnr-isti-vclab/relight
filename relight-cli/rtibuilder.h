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
	uint32_t samplingram = 500;
	uint32_t nsamples = 1<<16; //TODO change to rate
	float rangescale = 1.5;
	int skip_image = -1;
	//TODO: might want to use an euristic to get the best compromise to miniminze jpeg compression artifacts.
	float rangecompress = 0.5f; //betwee 0 and 1, where 0 is maximally compressed
	bool savenormals = false;
	bool savemeans = false;
	bool savemedians = false;
	int crop[4] = { 0, 0, 0, 0 }; //left, top, width, height

	std::function<bool(std::string stage, int percent)> *callback = nullptr;

	RtiBuilder();
	~RtiBuilder();
	bool initFromFolder(const std::string &folder, std::function<bool(std::string stage, int percent)> *_callback = nullptr);
	bool initFromProject(const std::string &filename, std::function<bool(std::string stage, int percent)> *_callback = nullptr);
	bool init(std::function<bool(std::string stage, int percent)> *_callback = nullptr);

	size_t save(const std::string &output, int quality = 95);
	bool saveJSON(QDir &dir, int quality);


	void processLine(int y, PixelArray &sample, PixelArray &resample, std::vector<std::vector<uint8_t>> &line,
					 std::vector<uchar> &normal, std::vector<uchar> &mean, std::vector<uchar> &median);

protected:
	//for each resample pos get coeffs from the origina lights.
	Resamplemap resamplemap;

	//grid of resamplemaps to be interpolated.
	int resample_width, resample_height;
	std::vector<Resamplemap> resamplemaps;  //for per pixel direction light interpolation

	Eigen::MatrixXd A;

	MaterialBuilder materialbuilder;

	void resamplePixel(Color3f *sample, Color3f *pixel, Vector3f pos);
	Vector3f getNormalThreeLights(std::vector<float> &pri); //use 3 virtual lights at 45 degs.

	void buildResampleMap(std::vector<Vector3f> &lights, std::vector<std::vector<std::pair<int, float> > > &remap);
	void buildResampleMaps();
	void remapPixel(Color3f *sample, Color3f *pixel, Resamplemap &resamplemap, float weight);


	void pickBase(PixelArray &sample);

	void estimateError(PixelArray &sample, std::vector<float> &weights);
	void getPixelMaterial(PixelArray &pixels, std::vector<size_t> &indices);
	void getPixelBestMaterial(PixelArray &pixels, std::vector<size_t> &indices);

	void pickBasePCA(PixelArray &sample);
	void pickBasePTM(std::vector<Vector3f> &lights);
	void pickBaseHSH(Type base = HSH);

	
//DEBUG
	
	void saveLightPixel(Color3f *p, int side, const QString &file);
	void savePixel(Color3f *p, int side, const QString &file);
	void debugMaterials();

	std::vector<float> toPrincipal(float *v);
};


#endif // RTIBUILDER_H
