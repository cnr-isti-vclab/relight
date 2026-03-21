#ifndef INIT_NORMALS_H
#define INIT_NORMALS_H

#include <vector>
#include <Eigen/Dense>
#include "../relight_vector.h"

namespace brdf {

struct InitNormalResult {
	Eigen::Vector3f normal;
	std::vector<float> shadow_probability; // per-light [0,1]: 0 = lit, 1 = likely shadow
	bool is_metallic;                      // true if the pixel looks metallic (few sharp reflections)
	float lambertian_variance;             // residual variance of the Lambertian fit
	float highlight_fraction;             // fraction of lights above the highlight threshold
	float peak_ratio;                     // max_lum / mean_lum
};

// Estimate initial normal for BRDF fitting.
//
// 1. Fit Lambertian photometric stereo (least-squares on all lights).
// 2. Compute residual variance of the fit.
// 3. Detect metallic pixels: very few lights with high intensity above the mean
//    indicate specular-dominated response. In that case, use the half-vector of
//    the brightest sample as the normal instead.
// 4. Detect shadows: for non-metallic pixels, samples that fall significantly
//    below the Lambertian prediction are marked with high shadow probability.
//
// p: pixel intensities already in [0,1].   L: light directions (unit vectors).
InitNormalResult init_normal(const Pixel& p, const std::vector<Eigen::Vector3f>& L);

} // namespace brdf

#endif // INIT_NORMALS_H
