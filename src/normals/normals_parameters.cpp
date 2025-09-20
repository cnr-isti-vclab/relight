#include "normals_parameters.h"

QString NormalsParameters::summary() {
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