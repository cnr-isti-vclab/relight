#ifndef NORMALS_PARAMETERS_H
#define NORMALS_PARAMETERS_H

#include <QString>

#include "../taskparameters.h"

enum NormalSolver { NORMALS_L2, NORMALS_SBL, NORMALS_RPCA };
enum FlatMethod { FLAT_NONE, FLAT_RADIAL, FLAT_FOURIER, FLAT_BLUR };
enum SurfaceIntegration { SURFACE_NONE, SURFACE_BNI, SURFACE_ASSM, SURFACE_FFT };

class NormalsParameters : public TaskParameters {
public:
	NormalsParameters() { path = "./"; }
	bool compute = true;
	QString input_path;

	NormalSolver solver = NORMALS_L2;

	FlatMethod flatMethod = FLAT_NONE;
	double fourierPercentage = 20;
	double blurPercentage = 10;

	SurfaceIntegration surface_integration = SURFACE_NONE;
	float bni_k = 2.0;
	float assm_error = 0.1;

	int surface_width = 0;
	int surface_height = 0;

	QString basename = "normals"; //filename for normals  img.

	QString summary() const override;
	QJsonObject toJson() const override;
};

#endif // NORMALS_PARAMETERS_H
