#ifndef RTIBUILDER_H
#define RTIBUILDER_H

#include <QString>

#include "../src/rti.h"
#include "../src/imageset.h"
#include "../src/material.h"

#ifdef USE_MATERIALS
#include <flann/flann.hpp>
#endif

#include <armadillo>

class QDir;


class RtiBuilder: public Rti {
public:
	ImageSet imageset;
	uint32_t samplingrate = 40;
	uint32_t nsamples = 1<<16; //TODO change to rate
	float rangescale = 1.5;

	RtiBuilder();
	~RtiBuilder();
	bool init(const std::string &folder);
	bool init();
	size_t save(const std::string &output, int quality = 95);
	bool saveJSON(QDir &dir, int quality);

protected:
	//for each resample pos get coeffs from the origina lights.
	std::vector<std::vector<std::pair<int, float>>> resamplemap;
	arma::Mat<double> A;

#ifdef USE_MATERIALS
	flann::Index<flann::L2<float>> *materialindex;
#endif

	std::vector<MaterialBuilder> materialbuilders;
	//std::vector<Material> materials;

	PixelArray resamplePixels(PixelArray &samples);
	void resamplePixel(Color3f *sample, Color3f *pixel);
	void buildResampleMap();
	void pickMaterials(PixelArray &sample);
	void pickBase(PixelArray &sample);

	void estimateError(PixelArray &sample, std::vector<size_t> &indices, std::vector<float> &weights);
	void getPixelMaterial(PixelArray &pixels, std::vector<size_t> &indices);
	void getPixelBestMaterial(PixelArray &pixels, std::vector<size_t> &indices);


	void pickBaseICA(PixelArray &sample, std::vector<size_t> &indices);
	void pickBasePCA(PixelArray &sample, std::vector<size_t> &indices);
	void pickBasePTM();
	void pickBaseHSH();

	
//DEBUG
	
	void saveLightPixel(Color3f *p, int side, const QString &file);
	void savePixel(Color3f *p, int side, const QString &file);
	void debugMaterials();

	std::vector<float> toPrincipal(uint32_t m, float *v);
};


#endif // RTIBUILDER_H
