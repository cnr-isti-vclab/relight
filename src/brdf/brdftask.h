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

class BrdfWorker
{
public:
	BrdfWorker(BrdfParameters _parameters, int _row, const PixelArray& toProcess, float* _normals, float* _albedo, float* _roughness, float* _f0, ImageSet &imageset, Lens &_lens) :
		parameters(_parameters), row(_row), m_Row(toProcess), normals(_normals), albedo(_albedo), roughness(_roughness), f0(_f0), m_Imageset(imageset) {
		m_Row.resize(toProcess.npixels(), toProcess.nlights);
		for(size_t i = 0; i < m_Row.size(); i++)
			m_Row[i] = toProcess[i];
	}

	void run() {
		std::vector<Eigen::Vector3f> L = m_Imageset.lights();
		int nth = (m_Row.nlights-1) * parameters.median_percentage / 100;
		if (nth < 0) nth = 0;
        if (nth >= m_Row.nlights) nth = m_Row.nlights - 1;

		for(size_t i = 0; i < m_Row.size(); i++) {
			Pixel p = m_Row[i];
			
			// Compute initial albedo (using Median approach as simplest heuristic)
			Eigen::Vector3f init_albedo_vec(0.1f, 0.1f, 0.1f);
			if(parameters.albedo == BrdfParameters::MEDIAN) {
				std::vector<float> c(p.size());
				for(int k = 0; k < 3; k++) {
					for(size_t j = 0; j < p.size(); j++) c[j] = p[j][k];
					std::nth_element(c.begin(), c.begin() + nth, c.end());
					init_albedo_vec[k] = std::max(c[nth] / 255.0f, 0.001f); 
				}
			}

			// Normalize observed pixel colors to [0,1] for mathematical BRDF model
			for(size_t j = 0; j < p.size(); j++) {
				p[j] = p[j] / 255.0f;
			}
			
			// Initialize Normals via simple Lambertian PS heuristics
			Eigen::VectorXf I_lum(p.size());
			Eigen::MatrixXf L_mat(p.size(), 3);
			for (size_t j = 0; j < p.size(); j++) {
				I_lum(j) = 0.2126f * p[j].r + 0.7152f * p[j].g + 0.0722f * p[j].b;
				L_mat.row(j) = L[j];
			}
			Eigen::Vector3f init_normal = brdf::simple_lambertian_photometric_stereo(I_lum, L_mat);
			init_normal.normalize();
			
			// Handle invalid or negative backward-facing normals
			if (std::isnan(init_normal.x()) || init_normal.z() < 0) {
				init_normal = Eigen::Vector3f(0.0f, 0.0f, 1.0f);
			}

			// Optmize
			brdf::BrdfFitResult result = brdf::optimize_brdf_pixel(
				p, L, init_normal, init_albedo_vec, 0.3f, 3.0f); // default rough=0.3, light_intensity=3.0 as python
			
			// Output maps with appropriate scaling to standard image arrays (0-255 bounds usually)
			if (normals) {
				normals[i*3 + 0] = std::clamp((result.normal.x() * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f);
				normals[i*3 + 1] = std::clamp((result.normal.y() * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f);
				normals[i*3 + 2] = std::clamp((result.normal.z() * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f);
			}
			if (albedo) {
				albedo[i*3 + 0] = std::clamp(result.albedo.x() * 255.0f, 0.0f, 255.0f);
				albedo[i*3 + 1] = std::clamp(result.albedo.y() * 255.0f, 0.0f, 255.0f);
				albedo[i*3 + 2] = std::clamp(result.albedo.z() * 255.0f, 0.0f, 255.0f);
			}
			if (roughness) {
				// Grey-scale 1-channel output
				roughness[i] = std::clamp(result.roughness * 255.0f, 0.0f, 255.0f);
			}
			if (f0) {
				f0[i*3 + 0] = std::clamp(result.f0.x() * 255.0f, 0.0f, 255.0f);
				f0[i*3 + 1] = std::clamp(result.f0.y() * 255.0f, 0.0f, 255.0f);
				f0[i*3 + 2] = std::clamp(result.f0.z() * 255.0f, 0.0f, 255.0f);
			}
		}
	}

private:
	BrdfParameters parameters;
	int row;
	PixelArray m_Row;

	float* normals = nullptr;
	float* albedo = nullptr;
	float* roughness = nullptr;
	float* f0 = nullptr;
	
	ImageSet &m_Imageset;
};

#endif // BRDFTASK_H
