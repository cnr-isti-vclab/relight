#ifndef BRDFTASK_H
#define BRDFTASK_H

#include "../task.h"
#include "../project.h"
#include "../imageset.h"
#include "../relight_vector.h"
#include "brdfparameters.h"

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

	void run() {
		int nth = (m_Row.nlights-1) * parameters.median_percentage / 100;
		assert(nth >= 0 && nth < m_Row.nlights);
		for(size_t i = 0; i < m_Row.size(); i++) {
			Pixel &p = m_Row[i];
			if(parameters.albedo == BrdfParameters::MEDIAN) {
				//separate components
				std::vector<float> c(p.size());
				for(int k = 0; k < 3; k++) {
					for(size_t j = 0; j < p.size(); j++) {
						c[j] = p[j][k];
					}
					std::nth_element(c.begin(), c.begin() + nth, c.end());
					albedo[i*3 + k] =  c[nth];
				}
			} else if(parameters.albedo == BrdfParameters::MEAN) {
				for(int k = 0; k < 3; k++) {
					albedo[i*3 + k] = 0.0f;
					for(size_t j = 0; j < p.size(); j++) {
						albedo[i*3 + k] += p[j][k];
					}
					albedo[i*3 + k] /= p.size();
				}
			}
		}
	}

private:
	BrdfParameters parameters;
	int row; //unused at the moment but miht be used at a later time.
	PixelArray m_Row;

	float *albedo = nullptr;
	ImageSet &m_Imageset;
	QMutex m_Mutex;
};


#endif // BRDFTASK_H
