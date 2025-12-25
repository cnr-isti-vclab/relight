#include "normals_parameters.h"

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
	obj["compute"] = compute;
	obj["inputPath"] = input_path;
	obj["solver"] = static_cast<int>(solver);
	obj["flatMethod"] = static_cast<int>(flatMethod);
	obj["flatPercentage"] = flatPercentage;
	obj["blurPercentage"] = blurPercentage;
	obj["surfaceIntegration"] = static_cast<int>(surface_integration);
	obj["bniK"] = bni_k;
	obj["assmError"] = assm_error;
	obj["surfaceWidth"] = surface_width;
	obj["surfaceHeight"] = surface_height;
	obj["basename"] = basename;
	return obj;
}