#ifndef BRDF_OPTIMIZER_H
#define BRDF_OPTIMIZER_H

#include <ostream>
#include <vector>
#include <Eigen/Dense>
#include "../relight_vector.h"

namespace brdf {

struct BrdfFitResult {
	Eigen::Vector3f normal;
	float roughness;
	float metallic;
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
// optimize_normal: If true, optimize the normal; otherwise keep it fixed
// optimize_albedo: If true, optimize the albedo; otherwise keep it fixed
// 
// Returns the fitted parameters.
BrdfFitResult optimize_brdf_pixel(const Pixel& I,
                                  const std::vector<Eigen::Vector3f>& L,
                                  const Eigen::Vector3f& initial_normal,
                                  const Eigen::Vector3f& initial_albedo,
                                  float initial_roughness, float initial_metallic,
                                  float light_intensity = 1.0f,
                                  bool optimize_normal = true,
                                  bool optimize_albedo = true);

// Fit a single shared material (roughness, metallic, albedo) to a pool of
// pixels that may have different surface normals.  Normals are fixed inputs
// (not optimized), allowing observations from a whole patch to constrain
// one consistent 'macro-pixel' material estimate.
//
// pixels  : one Pixel per patch position (K observations each)
// normals : per-pixel surface normal (size must equal pixels.size())
// L       : K light directions (shared across all pixels)
BrdfFitResult optimize_brdf_patch_material(
        const std::vector<Pixel>&           pixels,
        const std::vector<Eigen::Vector3f>& normals,
        const std::vector<Eigen::Vector3f>& L,
        const Eigen::Vector3f& initial_albedo,
        float initial_roughness,
        float initial_metallic,
        float light_intensity = 1.0f);

// Brute-forces BRDF parameters with the given step size and writes the 10 best
// candidates vs the Ceres solution to 'out'.
// Specular is treated as grey (1D) in the search space. Normal (upper-hemisphere
// 2D Cartesian grid) and albedo (full RGB, 3D) are only searched when the
// corresponding boolean is true; otherwise the Ceres values are used as fixed.
void brdf_bruteforce_compare(
		const Pixel& I,
		const std::vector<Eigen::Vector3f>& L,
		const BrdfFitResult& ceres_result,
		float light_intensity,
		int pixel_x, int pixel_y,
		std::ostream& out,
		float step = 0.1f,
		bool bruteforce_normal = false,
		bool bruteforce_albedo = false);

} // namespace brdf

#endif // BRDF_OPTIMIZER_H
