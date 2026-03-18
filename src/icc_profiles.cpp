#include "icc_profiles.h"

#include <QByteArray>
#include <QFile>
#include <QDebug>
#include <QString>

namespace {

std::vector<uint8_t> loadDisplayP3Profile() {
	QFile file(":/icc_profiles/Display P3.icc");
	if(!file.open(QIODevice::ReadOnly)) {
		throw QString("Unable to open Display P3 ICC profile" + file.fileName() + ':' + file.errorString());
	}
	QByteArray data = file.readAll();
	return std::vector<uint8_t>(data.begin(), data.end());
}

std::vector<uint8_t> createSRGBProfileData() {
	cmsHPROFILE profile = cmsCreate_sRGBProfile();
	cmsUInt32Number size = 0;
	cmsSaveProfileToMem(profile, nullptr, &size);
	std::vector<uint8_t> data(size);
	cmsSaveProfileToMem(profile, data.data(), &size);
	cmsCloseProfile(profile);
	return data;
}

std::vector<uint8_t> createLinearRGBProfileData() {
	cmsHPROFILE profile = cmsCreate_sRGBProfile();
	
	// Replace the tone curve of sRGB with a linear one
	cmsToneCurve* linearCurve = cmsBuildGamma(nullptr, 1.0);
	cmsToneCurve* curves[3] = { linearCurve, linearCurve, linearCurve };
	
	// Create a new profile with the same white point and primaries as sRGB but linear tone curve
	cmsCIExyY whitePoint;
	cmsWhitePointFromTemp(&whitePoint, 6504); // D65
	
	cmsCIExyYTRIPLE primaries = {
		{0.6400, 0.3300, 1.0}, // Red
		{0.3000, 0.6000, 1.0}, // Green
		{0.1500, 0.0600, 1.0}  // Blue
	};
	
	cmsHPROFILE linearProfile = cmsCreateRGBProfile(&whitePoint, &primaries, curves);
	
	cmsUInt32Number size = 0;
	cmsSaveProfileToMem(linearProfile, nullptr, &size);
	std::vector<uint8_t> data(size);
	cmsSaveProfileToMem(linearProfile, data.data(), &size);
	
	cmsCloseProfile(linearProfile);
	cmsCloseProfile(profile);
	cmsFreeToneCurve(linearCurve);
	
	return data;
}

}

const std::vector<uint8_t> &ICCProfiles::displayP3Data() {
	static std::vector<uint8_t> data = loadDisplayP3Profile();
	return data;
}

cmsHPROFILE ICCProfiles::openDisplayP3Profile() {
	const auto &data = displayP3Data();
	if(data.empty())
		return nullptr;
	return cmsOpenProfileFromMem(data.data(), data.size());
}

const std::vector<uint8_t> &ICCProfiles::sRGBData() {
	static std::vector<uint8_t> data = createSRGBProfileData();
	return data;
}

const std::vector<uint8_t> &ICCProfiles::linearRGBData() {
	static std::vector<uint8_t> data = createLinearRGBProfileData();
	return data;
}

cmsHPROFILE ICCProfiles::openLinearRGBProfile() {
	const auto &data = linearRGBData();
	if(data.empty())
		return nullptr;
	return cmsOpenProfileFromMem(data.data(), data.size());
}
