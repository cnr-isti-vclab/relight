#include "colorprofile.h"
#include "icc_profiles.h"

#include <lcms2.h>

namespace {

QString describeProfile(const std::vector<uint8_t> &profile_data, bool *is_rgb_out) {
	if(profile_data.empty())
		return "";

	cmsHPROFILE profile = cmsOpenProfileFromMem(profile_data.data(), profile_data.size());
	if(!profile)
		throw QString("Could not open icc profile data");
	cmsColorSpaceSignature colorspace = cmsGetColorSpace(profile);
	if(is_rgb_out)
		*is_rgb_out = (colorspace == cmsSigRgbData);

	char profile_desc[256] = {0};
	cmsGetProfileInfoASCII(profile, cmsInfoDescription, "en", "US", profile_desc, 255);
	cmsCloseProfile(profile);

	return QString(profile_desc);
}

}

QString ColorProfile::getProfileDescription(const std::vector<uint8_t> &profile_data, bool &is_rgb) {
	return describeProfile(profile_data, &is_rgb);
}

QString ColorProfile::getProfileDescription(const std::vector<uint8_t> &profile_data) {
	return describeProfile(profile_data, nullptr);
}

bool ColorProfile::isSRGBProfile(const std::vector<uint8_t> &profile_data) {
	bool is_rgb = false;
	QString desc = getProfileDescription(profile_data, is_rgb);
	return is_rgb && desc.contains("sRGB");
}


bool ColorProfile::isDisplayP3Profile(const std::vector<uint8_t> &profile_data) {
	bool is_rgb = false;
	QString desc = getProfileDescription(profile_data, is_rgb);
	return is_rgb && desc.contains("Display") && desc.contains("P3");
}

cmsHTRANSFORM ColorProfile::createColorTransform(const std::vector<uint8_t> &profile_data,
		ColorProfileMode mode,
		cmsHPROFILE &input_profile_handle) {
	if(profile_data.empty())
		throw QString("Color profile conversion requested but input images lack an embedded ICC profile.");
	cmsHPROFILE output_profile = createOutputProfile(mode);
	if(!output_profile)
		throw QString("Failed creating target ICC profile for color conversion.");
	if(input_profile_handle)
		cmsCloseProfile(input_profile_handle);
	input_profile_handle = cmsOpenProfileFromMem(profile_data.data(), profile_data.size());
	if(!input_profile_handle) {
		cmsCloseProfile(output_profile);
		throw QString("Failed opening source ICC profile for color conversion.");
	}
	cmsHTRANSFORM transform = cmsCreateTransform(input_profile_handle, TYPE_RGB_8, output_profile, TYPE_RGB_8, INTENT_PERCEPTUAL, cmsFLAGS_COPY_ALPHA);
	cmsCloseProfile(output_profile);
	if(!transform)
		throw QString("Failed creating ICC color transform for the requested profile.");
	return transform;
}

cmsHPROFILE ColorProfile::createOutputProfile(ColorProfileMode mode) {
	switch(mode) {
	case COLOR_PROFILE_SRGB:
		return cmsCreate_sRGBProfile();
	case COLOR_PROFILE_DISPLAY_P3:
		return ICCProfiles::openDisplayP3Profile();
	case COLOR_PROFILE_PRESERVE:
	default:
		return nullptr;
	}
}
