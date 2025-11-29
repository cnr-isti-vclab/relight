#include "colorprofile.h"

bool ColorProfile::isSRGBProfile(const std::vector<uint8_t> &profile_data) {
	if(profile_data.empty())
		return false;  // No profile means unknown, not sRGB
	
	cmsHPROFILE profile = cmsOpenProfileFromMem(profile_data.data(), profile_data.size());
	if(!profile)
		return false;
	
	cmsHPROFILE srgb = cmsCreate_sRGBProfile();
	
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
	
	cmsCloseProfile(srgb);
	cmsCloseProfile(profile);
	
	return is_srgb;
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
