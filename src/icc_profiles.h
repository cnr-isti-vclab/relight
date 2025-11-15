#ifndef ICC_PROFILES_H
#define ICC_PROFILES_H

#include <cstdint>
#include <cstddef>

// Standard sRGB ICC profile (v2, approximately 3KB)
// This is a minimal sRGB profile compatible with most software
extern const uint8_t sRGB_ICC_profile[];
extern const size_t sRGB_ICC_profile_length;

#endif // ICC_PROFILES_H
