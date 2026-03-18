#ifndef ICC_PROFILES_H
#define ICC_PROFILES_H

#include <cstdint>
#include <cstddef>
#include <vector>

#include <lcms2.h>

class ICCProfiles {
public:
	static const std::vector<uint8_t> &sRGBData();
	static const std::vector<uint8_t> &displayP3Data();
	static const std::vector<uint8_t> &linearRGBData();
	static cmsHPROFILE openDisplayP3Profile();
	static cmsHPROFILE openLinearRGBProfile();
};

#endif // ICC_PROFILES_H
