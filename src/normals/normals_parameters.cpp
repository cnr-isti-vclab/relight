#include "normals_parameters.h"

namespace {

QString solverToString(NormalSolver solver) {
	switch(solver) {
	case NORMALS_L2: return QStringLiteral("NORMALS_L2");
	case NORMALS_SBL: return QStringLiteral("NORMALS_SBL");
	case NORMALS_RPCA: return QStringLiteral("NORMALS_RPCA");
	case NORMALS_ROBUST: return QStringLiteral("NORMALS_ROBUST");
	default:
		return QStringLiteral("UNKNOWN");
	}
}

QString flatMethodToString(FlatMethod method) {
	switch(method) {
	case FLAT_NONE: return QStringLiteral("FLAT_NONE");
	case FLAT_RADIAL: return QStringLiteral("FLAT_RADIAL");
	case FLAT_PLANE: return QStringLiteral("FLAT_PLANE");
	case FLAT_FOURIER: return QStringLiteral("FLAT_FOURIER");
	case FLAT_BLUR: return QStringLiteral("FLAT_BLUR");
	default:
		return QStringLiteral("UNKNOWN");
	}
}

QString surfaceIntegrationToString(SurfaceIntegration mode) {
	switch(mode) {
	case SURFACE_NONE: return QStringLiteral("SURFACE_NONE");
	case SURFACE_BNI: return QStringLiteral("SURFACE_BNI");
	case SURFACE_ASSM: return QStringLiteral("SURFACE_ASSM");
	case SURFACE_FFT: return QStringLiteral("SURFACE_FFT");
	default:
		return QStringLiteral("UNKNOWN");
	}
}

QString normalmapOutputToString(NormalmapOutput o) {
	switch(o) {
	case NORMAL_OUT_JPEG: return QStringLiteral("JPEG");
	case NORMAL_OUT_PNG: return QStringLiteral("PNG");
	case NORMAL_OUT_JPEGXL: return QStringLiteral("JPEGXL");
	case NORMAL_OUT_EXR: return QStringLiteral("EXR");
	case NORMAL_OUT_TIFF: return QStringLiteral("TIFF");
	default: return QStringLiteral("UNKNOWN");
	}
}

QString depthmapOutputToString(DepthmapOutput o) {
	switch(o) {
	case DEPTH_OUT_JPEGXL: return QStringLiteral("JPEGXL");
	case DEPTH_OUT_EXR: return QStringLiteral("EXR");
	case DEPTH_OUT_TIFF: return QStringLiteral("TIFF");
	default: return QStringLiteral("UNKNOWN");
	}
}

QString surfaceOutputToString(SurfaceOutput o) {
	switch(o) {
	case SURFACE_OUT_PLY: return QStringLiteral("PLY");
	case SURFACE_OUT_OBJ: return QStringLiteral("OBJ");
	case SURFACE_OUT_GLTF: return QStringLiteral("GLTF");
	default: return QStringLiteral("UNKNOWN");
	}
}

}

QString NormalsParameters::summary() const {
	QString ret = "Normals";
	if(flatMethod == FLAT_RADIAL)
		ret += " , radial flattning";
	if(flatMethod == FLAT_PLANE)
		ret += ", 4-point plane flattening";
	if(flatMethod == FLAT_FOURIER)
		ret += ", frequencies based flattening";

	if(surface_integration == SURFACE_ASSM)
		ret += ", adaptive surface reconstruction";
	if(surface_integration == SURFACE_BNI)
		ret += ", bilateral surface reconstruction";
	if(surface_integration == SURFACE_FFT)
		ret += ", Fourier transform surface reconstruction";
	ret += ".";
	return ret;
}

QJsonObject NormalsParameters::toJson() const {
	QJsonObject obj = baseJson();
	obj["taskType"] = "NORMALS";
	obj["compute"] = compute;
	obj["inputPath"] = input_path;
	obj["solver"] = solverToString(solver);
	obj["robustThresholdHigh"] = robust_threshold_high;
	obj["robustThresholdLow"]  = robust_threshold_low;
	obj["flatMethod"] = flatMethodToString(flatMethod);
	obj["flatPercentage"] = fourierPercentage;
	obj["blurPercentage"] = blurPercentage;
	obj["surfaceIntegration"] = surfaceIntegrationToString(surface_integration);
	obj["bniK"] = bni_k;
	obj["assmError"] = assm_error;
	obj["surfaceWidth"] = surface_width;
	obj["surfaceHeight"] = surface_height;
	obj["normalsname"] = normalsname;
	obj["normalmapOutput"] = normalmapOutputToString(normalmap_output);
	obj["depthmapOutput"] = depthmapOutputToString(depthmap_output);
	obj["surfaceOutput"] = surfaceOutputToString(surface_output);
	obj["depthNormalize"] = depth_normalize;
	return obj;
}
