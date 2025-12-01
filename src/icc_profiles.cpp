#include "icc_profiles.h"

// Embedded sRGB IEC61966-2.1 profile extracted from /usr/share/color/icc/colord/sRGB.icc.
// Display P3 profile generated via lcms (DisplayP3.icc kept in repo for future bundling).

#include "icc_profiles_data.inc"
#include "icc_profile_displayp3.inc"

const size_t sRGB_ICC_profile_length = sizeof(sRGB_ICC_profile);
const size_t DisplayP3_ICC_profile_length = sizeof(DisplayP3_ICC_profile);
