#include "colorprofile.h"

bool ColorProfile::isSRGBProfile(const std::vector<uint8_t> &profile_data) {
	if(profile_data.empty())
		return false;  // No profile means unknown, not sRGB
	
	cmsHPROFILE profile = cmsOpenProfileFromMem(profile_data.data(), profile_data.size());
	if(!profile)
		return false;
	
	// Compare profile info
	cmsColorSpaceSignature colorspace = cmsGetColorSpace(profile);
	bool is_rgb = (colorspace == cmsSigRgbData);
	
	// Check if profiles are similar by comparing key properties
	bool is_srgb = false;
	if(is_rgb) {
		// Get profile description
		char profile_desc[256] = {0};
		cmsGetProfileInfoASCII(profile, cmsInfoDescription, "en", "US", profile_desc, 255);
		QString desc = QString(profile_desc).toLower();
		
		// Check if description contains sRGB indicators
		is_srgb = desc.contains("srgb") || desc.contains("s rgb") || desc.contains("iec61966");
	}
	cmsCloseProfile(profile);
	
	return is_srgb;
}

bool ColorProfile::isDisplayP3Profile(const std::vector<uint8_t> &profile_data) {
	if(profile_data.empty())
		return false;

	cmsHPROFILE profile = cmsOpenProfileFromMem(profile_data.data(), profile_data.size());
	if(!profile)
		return false;

	char profile_desc[256] = {0};
	cmsGetProfileInfoASCII(profile, cmsInfoDescription, "en", "US", profile_desc, 255);
	QString desc = QString(profile_desc).toLower();
	cmsColorSpaceSignature colorspace = cmsGetColorSpace(profile);
	cmsCloseProfile(profile);

	return colorspace == cmsSigRgbData &&
		(desc.contains("display p3") || desc.contains("display-p3") || desc.contains("p3 display"));
}

QString ColorProfile::getProfileDescription(const std::vector<uint8_t> &profile_data) {
	if(profile_data.empty())
		return "No profile";
	
	cmsHPROFILE profile = cmsOpenProfileFromMem(profile_data.data(), profile_data.size());
	if(!profile)
		return "Invalid profile";
	
	char profile_desc[256] = {0};
	cmsGetProfileInfoASCII(profile, cmsInfoDescription, "en", "US", profile_desc, 255);
	
	cmsCloseProfile(profile);
	
	return QString(profile_desc);
}
