#ifndef BRDFTASK_H
#define BRDFTASK_H

#include "../task.h"
#include "../project.h"
#include "../imageset.h"
#include "../relight_vector.h"
#include "brdfparameters.h"
#include "brdf_optimizer.h"
#include "brdf_math.h"
#include <fstream>

//TODO: this is the same as normalstask!
class BrdfTask :  public Task {
public:
	BrdfParameters parameters;

	ImageSet imageset;
	Lens lens;

	virtual void run() override;
	virtual QJsonObject info() const override;

	void setParameters(BrdfParameters &param);
	void initFromProject(Project &project);
	void initFromFolder(const char *folder, Dome &dome, const Crop &crop);
};

class AlbedoWorker
{
public:
	AlbedoWorker(BrdfParameters _parameters, int _row, const PixelArray& toProcess,
				 uint8_t* _albedo_out, ImageSet &imageset, Lens &_lens) :
		parameters(_parameters), row(_row), m_Row(toProcess), albedo_out(_albedo_out), m_Imageset(imageset) {

		m_Row.resize(toProcess.npixels(), toProcess.nlights);
		for(size_t i = 0; i < m_Row.size(); i++)
			m_Row[i] = toProcess[i];
	}

	void run();


private:
	BrdfParameters parameters;
	int row;
	PixelArray m_Row;

	cmsHTRANSFORM output_color_transform_float = nullptr;
	uint8_t *albedo_out = nullptr;
	ImageSet &m_Imageset;
	QMutex m_Mutex;
};

class BrdfWorker
{
public:
	BrdfWorker(BrdfParameters _parameters, int _row, const PixelArray& toProcess, float* _normals, float* _albedo, float* _roughness, float* _specular, ImageSet &imageset, Lens &_lens,
			   std::ofstream* _brute_out = nullptr, QMutex* _brute_mutex = nullptr, QString _plot_dir = QString(),
			   float* _metallic_mask = nullptr, float* _highlight_frac = nullptr,
			   float* _peak_ratio_data = nullptr, uint8_t* _shadow_data = nullptr, int _nlights_count = 0) :
		parameters(_parameters), row(_row), m_Row(toProcess), normals(_normals), albedo(_albedo), roughness(_roughness), specular(_specular), m_Imageset(imageset),
		brute_out(_brute_out), brute_mutex(_brute_mutex), plot_dir(_plot_dir),
		metallic_mask(_metallic_mask), highlight_frac(_highlight_frac),
		peak_ratio_data(_peak_ratio_data), shadow_data(_shadow_data), nlights_count(_nlights_count) {
		m_Row.resize(toProcess.npixels(), toProcess.nlights);
		for(size_t i = 0; i < m_Row.size(); i++)
			m_Row[i] = toProcess[i];
	}

	void run();

private:
	BrdfParameters parameters;
	int row;
	PixelArray m_Row;

	float* normals = nullptr;
	float* albedo = nullptr;
	float* roughness = nullptr;
	float* specular = nullptr;

	std::ofstream* brute_out = nullptr;
	QMutex* brute_mutex = nullptr;
	QString plot_dir;

	float* metallic_mask = nullptr;
	float* highlight_frac = nullptr;
	float* peak_ratio_data = nullptr;
	uint8_t* shadow_data = nullptr;
	int nlights_count = 0;

	ImageSet &m_Imageset;
};

#endif // BRDFTASK_H
