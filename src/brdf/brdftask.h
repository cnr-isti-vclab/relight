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
	AlbedoWorker(BrdfParameters _parameters, int _row, const PixelArray& toProcess,
	             cmsHTRANSFORM _output_color_transform_float, uint8_t* _albedo_out,
	             ImageSet &imageset, Lens &_lens) :
		parameters(_parameters), row(_row), m_Row(toProcess),
		output_color_transform_float(_output_color_transform_float),
		albedo_out(_albedo_out), m_Imageset(imageset) {

		m_Row.resize(toProcess.npixels(), toProcess.nlights);
		for(size_t i = 0; i < m_Row.size(); i++)
			m_Row[i] = toProcess[i];
	}

	void run() {
		int nth = (m_Row.nlights-1) * parameters.median_percentage / 100;
		assert(nth >= 0 && nth < m_Row.nlights);
		for(size_t i = 0; i < m_Row.size(); i++) {
			Pixel &p = m_Row[i];
			float rgb[3] = {0.0f, 0.0f, 0.0f};
			if(parameters.albedo == BrdfParameters::MEDIAN) {
				std::vector<float> c(p.size());
				for(int k = 0; k < 3; k++) {
					for(size_t j = 0; j < p.size(); j++)
						c[j] = p[j][k];
					std::nth_element(c.begin(), c.begin() + nth, c.end());
					rgb[k] = c[nth];
				}
			} else if(parameters.albedo == BrdfParameters::MEAN) {
				for(int k = 0; k < 3; k++) {
					for(size_t j = 0; j < p.size(); j++)
						rgb[k] += p[j][k];
					rgb[k] /= p.size();
				}
			}

			if(output_color_transform_float) {
				float norm[3] = {
					std::max(0.0f, std::min(1.0f, rgb[0] / 255.0f)),
					std::max(0.0f, std::min(1.0f, rgb[1] / 255.0f)),
					std::max(0.0f, std::min(1.0f, rgb[2] / 255.0f))
				};
				cmsDoTransform(output_color_transform_float, norm, &albedo_out[i*3], 1);
			} else {
				for(int k = 0; k < 3; k++)
					albedo_out[i*3 + k] = (uint8_t)std::min(std::max(int(rgb[k]), 0), 255);
			}
		}
	}

private:
	BrdfParameters parameters;
	int row;
	PixelArray m_Row;

	cmsHTRANSFORM output_color_transform_float = nullptr;
	uint8_t *albedo_out = nullptr;
	ImageSet &m_Imageset;
	QMutex m_Mutex;
};


#endif // BRDFTASK_H
