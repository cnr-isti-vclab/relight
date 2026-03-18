#ifndef BRDFTASK_H
#define BRDFTASK_H

#include "../task.h"
#include "../project.h"
#include "../imageset.h"
#include "../relight_vector.h"
#include "brdfparameters.h"
#include "brdf_optimizer.h"
#include "brdf_math.h"

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
	AlbedoWorker(BrdfParameters _parameters, int _row, const PixelArray& toProcess, float* _albedo, ImageSet &imageset, Lens &_lens) :
		parameters(_parameters), row(_row), m_Row(toProcess), albedo(_albedo), m_Imageset(imageset) {
		m_Row.resize(toProcess.npixels(), toProcess.nlights);
		for(size_t i = 0; i < m_Row.size(); i++)
			m_Row[i] = toProcess[i];
	}

	void run();

private:
	BrdfParameters parameters;
	int row; //unused at the moment but miht be used at a later time.
	PixelArray m_Row;

	float *albedo = nullptr;
	ImageSet &m_Imageset;
	QMutex m_Mutex;
};

class BrdfWorker
{
public:
	BrdfWorker(BrdfParameters _parameters, int _row, const PixelArray& toProcess, float* _normals, float* _albedo, float* _roughness, float* _specular, ImageSet &imageset, Lens &_lens) :
		parameters(_parameters), row(_row), m_Row(toProcess), normals(_normals), albedo(_albedo), roughness(_roughness), specular(_specular), m_Imageset(imageset) {
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
	
	ImageSet &m_Imageset;
};

#endif // BRDFTASK_H
