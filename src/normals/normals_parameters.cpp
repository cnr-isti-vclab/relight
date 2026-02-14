#include "normals_parameters.h"

namespace {

QString solverToString(NormalSolver solver) {
	switch(solver) {
	case NORMALS_L2: return QStringLiteral("NORMALS_L2");
	case NORMALS_SBL: return QStringLiteral("NORMALS_SBL");
	case NORMALS_RPCA: return QStringLiteral("NORMALS_RPCA");
	default:
		return QStringLiteral("UNKNOWN");
	}
}

QString flatMethodToString(FlatMethod method) {
	switch(method) {
	case FLAT_NONE: return QStringLiteral("FLAT_NONE");
	case FLAT_RADIAL: return QStringLiteral("FLAT_RADIAL");
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

}

QString NormalsParameters::summary() const {
	QString ret = "Normals";
	if(flatMethod == FLAT_RADIAL)
		ret += " , radial flattning";
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
	obj["flatMethod"] = flatMethodToString(flatMethod);
	obj["flatPercentage"] = fourierPercentage;
	obj["blurPercentage"] = blurPercentage;
	obj["surfaceIntegration"] = surfaceIntegrationToString(surface_integration);
	obj["bniK"] = bni_k;
	obj["assmError"] = assm_error;
	obj["surfaceWidth"] = surface_width;
	obj["surfaceHeight"] = surface_height;
	obj["normalsname"] = normalsname;
	return obj;
}
