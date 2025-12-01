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
	static cmsHPROFILE openDisplayP3Profile();
};

#endif // ICC_PROFILES_H
