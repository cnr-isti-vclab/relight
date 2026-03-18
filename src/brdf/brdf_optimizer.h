#ifndef BRDF_OPTIMIZER_H
#define BRDF_OPTIMIZER_H

#include <vector>
#include <Eigen/Dense>
#include "../relight_vector.h"

namespace brdf {

struct BrdfFitResult {
    Eigen::Vector3f normal;
    float roughness;
    Eigen::Vector3f f0;
    Eigen::Vector3f albedo; // Passed through, but stored for convenience
};

// Optimizes the microfacet BRDF model for a single pixel using Ceres.
// 
// I: Intensity list (K items, 3 channels) as read from ImageSet
// L: Direction list (K directions)
// initial_normal: The starting estimate (e.g. from photometric stereo)
// initial_albedo: Precalculated albedo (fixed during optimization)
// initial_roughness: The starting roughness guess
// light_intensity: Master scalar intensity of lights
// 
// Returns the fitted parameters.
BrdfFitResult optimize_brdf_pixel(
    const Pixel& I, 
    const std::vector<Eigen::Vector3f>& L, 
    const Eigen::Vector3f& initial_normal,
    const Eigen::Vector3f& initial_albedo,
    float initial_roughness,
    float light_intensity = 3.0f);

} // namespace brdf

#endif // BRDF_OPTIMIZER_H
