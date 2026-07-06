#ifndef NORMALS_PARAMETERS_H
#define NORMALS_PARAMETERS_H

#include <QString>
#include <QPointF>
#include <vector>

#include "../taskparameters.h"

enum NormalSolver { NORMALS_L2, NORMALS_SBL, NORMALS_RPCA, NORMALS_ROBUST };
enum NormalFormat { NORMAL_OPENGL, NORMAL_DIRECTX };
enum FlatMethod { FLAT_NONE, FLAT_RADIAL, FLAT_PLANE, FLAT_FOURIER, FLAT_BLUR };
enum SurfaceIntegration { SURFACE_NONE, SURFACE_BNI, SURFACE_ASSM, SURFACE_FFT };

// Normalmap output formats. Note: JPEGXL/EXR/TIFF are intended as 16-bit outputs.
enum NormalmapOutput { NORMAL_OUT_JPEG, NORMAL_OUT_PNG, NORMAL_OUT_JPEGXL, NORMAL_OUT_EXR, NORMAL_OUT_TIFF };

// Depthmap output formats. EXR and TIFF are floating-point; JPEGXL is lossy/lossless compressed float-capable.
enum DepthmapOutput { DEPTH_OUT_JPEGXL, DEPTH_OUT_EXR, DEPTH_OUT_TIFF };

// Surface export formats
enum SurfaceOutput { SURFACE_OUT_PLY, SURFACE_OUT_OBJ, SURFACE_OUT_GLTF };

class NormalsParameters : public TaskParameters {
public:
	NormalsParameters() { path = "./"; }
	bool compute = true;
	QString input_path;

	NormalSolver solver = NORMALS_L2;
	NormalFormat normalFormat = NORMAL_OPENGL;

	// 4-point plane flattening: points in full-project-image pixel coords
	std::vector<QPointF> plane_points;

	// Robust solver thresholds (pixel intensity in [0, 255]).
	// Values above high_threshold are treated as specular highlights and excluded.
	// Values below low_threshold are treated as shadows and excluded.
	float robust_threshold_high = 255.0f;
	float robust_threshold_low  =   5.0f;

	FlatMethod flatMethod = FLAT_NONE;
	double fourierPercentage = 20;
	double blurPercentage = 10;

	SurfaceIntegration surface_integration = SURFACE_NONE;
	float bni_k = 0.0;
	float assm_error = 0.1;

	int surface_width = 0;  //3d surface grid width after downsampling.
	int surface_height = 0;

	// Output format selections
	NormalmapOutput normalmap_output = NORMAL_OUT_JPEG;
	DepthmapOutput depthmap_output = DEPTH_OUT_TIFF;
	SurfaceOutput surface_output = SURFACE_OUT_PLY;

	// When true, also save a normalized depthmap (0..1) in addition to raw depth
	bool depth_normalize = false;

	QString normalsname = "normals"; //filename for normals img.

	QString summary() const override;
	QJsonObject toJson() const override;
};

#endif // NORMALS_PARAMETERS_H
