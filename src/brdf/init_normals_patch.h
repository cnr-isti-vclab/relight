#ifndef INIT_NORMALS_PATCH_H
#define INIT_NORMALS_PATCH_H

#include <vector>
#include <Eigen/Dense>
#include "../relight_vector.h"
#include "init_normals.h"

namespace brdf {

// Default GGX roughness basis used by init_normal_patch().
// Four lobes cover the range from sharp specular to near-Lambertian.
inline std::vector<double> defaultPatchAlphas() {
    return { 0.08, 0.25, 0.55, 0.90 };
}

struct PatchInitResult {
    // Per-pixel results — one entry per pixel in the patch (row-major).
    // normals[i] is the refined surface normal for window pixel i.
    // seed_results[i] is the fast single-pixel Lambertian estimate (warm-start).
    std::vector<Eigen::Vector3f> normals;
    std::vector<InitNormalResult> seed_results;

    // Shared GGX lobe weights (size == alphas.size()).
    std::vector<double> weights;
    std::vector<double> alphas;

    bool success = false;
};

// Jointly fit surface normals for all pixels in a patch, sharing the BRDF.
//
// patch    : P pixels in row-major order (P = wsize*wsize).
// lights   : K light directions (same for every pixel).
// alphas   : J fixed GGX roughness values for the basis; use
//            defaultPatchAlphas() for a sensible default.
//
// Algorithm:
//   1. Run single-pixel init_normal() on each patch pixel as a fast warm-start.
//   2. Feed warm-start normals + uniform weights into SolvePatch(), which
//      jointly optimises all P normals and J shared weights via Ceres.
//
// The center pixel index is  (wsize/2)*wsize + wsize/2  in row-major order,
// where wsize = sqrt(P) (assumed square, odd).
PatchInitResult init_normal_patch(
        const std::vector<Pixel>&           patch,
        const std::vector<Eigen::Vector3f>& lights,
        const std::vector<double>&          alphas = {});

} // namespace brdf

#endif // INIT_NORMALS_PATCH_H
