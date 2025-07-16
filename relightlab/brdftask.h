#ifndef BRDFTASK_H
#define BRDFTASK_H

#include "task.h"
#include "../src/project.h"
#include "../src/imageset.h"
#include "../src/relight_vector.h"

class BrdfParameters {
public:
	QString input_path;
	enum AlbedoMethod { NONE, MEDIAN };
	AlbedoMethod albedo = MEDIAN;
	float median_percentage = 66;

	int quality = 95;
	QString path;

	QString summary();
};

//TODO: this is the same as normalstask!
class BrdfTask :  public Task {
public:
	BrdfParameters parameters;

	ImageSet imageset;
	Crop crop;
	QSize img_size;

	Lens lens;
	float pixelSize = 0.0f;

	virtual void run() override;

	void setParameters(BrdfParameters &param);
	void initFromProject(Project &project);
};

class MedianWorker
{
public:
	MedianWorker(BrdfParameters _parameters, int _row, const PixelArray& toProcess, float* _albedo, ImageSet &imageset, Lens &_lens) :
		parameters(_parameters), row(_row), m_Row(toProcess), albedo(_albedo), m_Imageset(imageset), lens(_lens){
		m_Row.resize(toProcess.npixels(), toProcess.nlights);
		for(size_t i = 0; i < m_Row.size(); i++)
			m_Row[i] = toProcess[i];
	}

	void run() {
		int nth = (m_Row.nlights-1) * parameters.median_percentage / 100;
		assert(nth >= 0 && nth < m_Row.nlights);
		for(size_t i = 0; i < m_Row.size(); i++) {
			Pixel &p = m_Row[i];
			//separate components
			std::vector<float> c(p.size());
			for(int k = 0; k < 3; k++) {
				for(int j = 0; j < p.size(); j++) {
					c[j] = p[j][k];
				}
				std::nth_element(c.begin(), c.begin() + nth, c.end());
				albedo[i*3 + k] =  c[nth];
			}
		}
	}

private:
	BrdfParameters parameters;
	int row;
	PixelArray m_Row;

	float *albedo = nullptr;
	ImageSet &m_Imageset;
	Lens &lens;
	QMutex m_Mutex;
};


#endif // BRDFTASK_H
