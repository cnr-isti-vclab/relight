#ifndef COLORPROFILE_H
#define COLORPROFILE_H

#include <QString>
#include <vector>
#include <cstdint>
#include <lcms2.h>

// ICC color profile handling mode
enum ColorProfileMode {
	COLOR_PROFILE_PRESERVE,  // Pass through input ICC profile
	COLOR_PROFILE_SRGB,      // Convert to sRGB (requires LittleCMS2)
	COLOR_PROFILE_DISPLAY_P3 // Convert to Display P3 (requires LittleCMS2)
};

class ColorProfile {
public:
	// Check if a profile data is sRGB
	static bool isSRGBProfile(const std::vector<uint8_t> &profile_data);
	static bool isDisplayP3Profile(const std::vector<uint8_t> &profile_data);
	
	// Get human-readable profile description
	static QString getProfileDescription(const std::vector<uint8_t> &profile_data);
};

#endif // COLORPROFILE_H
