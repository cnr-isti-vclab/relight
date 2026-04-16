#ifndef NORMALSTASK_H
#define NORMALSTASK_H

#include "../task.h"
#include "../project.h"
#include "../imageset.h"
#include "../jpeg_encoder.h"
#include "normals_parameters.h"
#include "normalsworker.h"

#include <QJsonObject>
#include <QMutex>
#include <QRect>
#include <QRunnable>
#include <QString>

#include <Eigen/Core>

#include <functional>


class NormalsTask :  public Task {
public:
	NormalsParameters parameters;

	ImageSet imageset;
	Lens lens;
	float z_threshold =0.001;

	virtual void run() override;
	virtual QJsonObject info() const override;

	void setParameters(NormalsParameters &param);
	void initFromProject(Project &project);
	void initFromFolder(const char *folder, Dome &dome, const Crop &crop);

	void assm(QString filename, std::vector<Eigen::Vector3f> &normals, int width, int height, float precision,
			  std::function<bool(QString, int)> *_callback);
	void fixNormal(Eigen::Vector3f &n); //check for nan, and z< threshold

	// Debug: compute shadow masks for every pixel/light and save a side-by-side
	// PNG per light into destination: left half = original image, right half = mask
	// (black = Lambertian-consistent, white = shadow / highlight).
	void saveShadowDebug(QDir &destination, int width, int height);

	// Weight debug: one grayscale image per light (pixel brightness = solver
	// weight for that light) plus a mean-weight image.  Call initWeightsDebug
	// before the compute loop, writeWeightsDebugRow after each worker completes,
	// and finishWeightsDebug once all rows are done.
	void initWeightsDebug(QDir &destination, int width, int height);
	void writeWeightsDebugRow(const float *weights, const PixelArray &line, int nP);
	void finishWeightsDebug();

private:
	std::vector<JpegEncoder*> m_wdEncoders;   // one per light
	JpegEncoder*              m_wdMeanEncoder = nullptr;
	int                       m_wdNLights = 0;
	int                       m_wdWidth   = 0;
};

#endif // NORMALSTASK_H
