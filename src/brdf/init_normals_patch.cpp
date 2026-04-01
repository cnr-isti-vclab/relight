#include "init_normals_patch.h"
#include "brdf_bivariate.h"

#include <algorithm>
#include <cmath>

namespace brdf {

PatchInitResult init_normal_patch(
        const std::vector<Pixel>&           patch,
        const std::vector<Eigen::Vector3f>& lights,
        const std::vector<double>&          alphas_in)
{
    PatchInitResult result;
    result.alphas  = alphas_in.empty() ? defaultPatchAlphas() : alphas_in;
    const int P    = int(patch.size());
    const int K    = int(lights.size());
    const int J    = int(result.alphas.size());

    if (P == 0 || K == 0) {
        result.success = false;
        return result;
    }

    // -----------------------------------------------------------------------
    // Step 1: per-pixel Lambertian warm-start
    // -----------------------------------------------------------------------
    result.seed_results.resize(P);
    std::vector<Eigen::Vector3d> patch_normals(P);

    for (int p = 0; p < P; ++p) {
        result.seed_results[p] = init_normal(patch[p], lights);
        const Eigen::Vector3f& n = result.seed_results[p].normal;
        patch_normals[p]         = n.cast<double>();
    }

    // -----------------------------------------------------------------------
    // Step 2: build observations array [P * K] (luminance)
    // -----------------------------------------------------------------------
    std::vector<double> observations(P * K);
    for (int p = 0; p < P; ++p) {
        const Pixel& px = patch[p];
        for (int k = 0; k < K; ++k) {
            const Color3f& c = px[k];
            observations[p * K + k] =
                double(0.299f * c.r + 0.587f * c.g + 0.114f * c.b);
        }
    }

    // Convert lights to double
    std::vector<Eigen::Vector3d> lights_d(K);
    for (int k = 0; k < K; ++k)
        lights_d[k] = lights[k].cast<double>();

    // -----------------------------------------------------------------------
    // Step 3: joint patch solve
    // -----------------------------------------------------------------------
    // Warm-start weights: equal, normalised so their sum matches the median
    // observed luminance.
    double median_lum = 0.0;
    {
        std::vector<double> tmp(observations);
        std::nth_element(tmp.begin(), tmp.begin() + int(tmp.size()) / 2, tmp.end());
        median_lum = tmp[size_t(int(tmp.size()) / 2)];
    }
    double w0 = std::max(median_lum / std::max(J, 1), 1e-4);
    result.weights.assign(J, w0);

    SolvePatch(lights_d, observations, result.alphas, patch_normals, result.weights);

    // -----------------------------------------------------------------------
    // Step 4: copy results back (cast double→float, re-normalise)
    // -----------------------------------------------------------------------
    result.normals.resize(P);
    for (int p = 0; p < P; ++p) {
        Eigen::Vector3f n = patch_normals[p].cast<float>();
        float norm = n.norm();
        if (norm > 1e-6f)
            n /= norm;
        else
            n = result.seed_results[p].normal;
        if (n.z() < 0.0f) n = -n;
        result.normals[p] = n;
    }

    result.success = true;
    return result;
}

} // namespace brdf
