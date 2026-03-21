#include "init_normals.h"
#include "brdf_math.h"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace brdf {

InitNormalResult init_normal(const Pixel& p, const std::vector<Eigen::Vector3f>& L) {
    const Eigen::Vector3f V(0.0f, 0.0f, 1.0f);
    const size_t K = p.size();

    InitNormalResult result;
    result.shadow_probability.resize(K, 0.0f);
    result.is_metallic = false;
    result.lambertian_variance = 0.0f;

    // --- Step 1: Compute per-light luminance ---
    std::vector<float> lum(K);
    for (size_t j = 0; j < K; j++)
        lum[j] =  (p[j].r + p[j].g  + p[j].b)/3.0f;

    // --- Step 2: Lambertian photometric stereo (least-squares) ---
    Eigen::VectorXf I_lum = Eigen::Map<const Eigen::VectorXf>(lum.data(), K);
    Eigen::MatrixXf L_mat(K, 3);
    for (size_t j = 0; j < K; j++)
        L_mat.row(j) = L[j];

    Eigen::Vector3f lamb_n = simple_lambertian_photometric_stereo(I_lum, L_mat);
    float lamb_albedo = lamb_n.norm();
    if (lamb_albedo > 1e-6f || std::isnan(lamb_n.x()) || lamb_n.z() < 0)
        lamb_n.normalize();
    else
        lamb_n = Eigen::Vector3f(0.0f, 0.0f, 1.0f);

    // Lambertian prediction per light: I_pred = albedo * max(N·L, 0)
    std::vector<float> pred(K);
    float residual_sum = 0.0f;
    for (size_t j = 0; j < K; j++) {
        float ndotl = std::max(lamb_n.dot(L[j]), 0.0f);
        pred[j] = lamb_albedo * ndotl;
        float diff = lum[j] - pred[j];
        residual_sum += diff * diff;
    }
    result.lambertian_variance = residual_sum / std::max(K, (size_t)1);

    // --- Step 3: Detect metallic (specular-dominated) pixels ---
    // Metallic surfaces have very few lights with intensity well above the
    // Lambertian prediction (sharp specular spikes), while most lights are dim.
    float mean_lum = std::accumulate(lum.begin(), lum.end(), 0.0f) / K;
    float max_lum = *std::max_element(lum.begin(), lum.end());

    // Count how many lights are significantly above the mean (specular highlights)
    float highlight_threshold = mean_lum + 4.0f * std::sqrt(result.lambertian_variance);
    int highlight_count = 0;
    for (size_t j = 0; j < K; j++) {
        if (lum[j] > highlight_threshold)
            highlight_count++;
    }

    // Metallic heuristic: few bright highlights, large peak-to-mean ratio
    float peak_ratio = (mean_lum > 1e-6f) ? max_lum / mean_lum : 1.0f;
    float highlight_fraction = float(highlight_count) / K;
    result.highlight_fraction = highlight_fraction;
    result.peak_ratio = peak_ratio;
    if (highlight_fraction < 0.15f && peak_ratio > 3.0f) {
        result.is_metallic = true;
        // Use half-vector of brightest sample as normal
        size_t max_idx = std::max_element(lum.begin(), lum.end()) - lum.begin();
        result.normal = (L[max_idx] + V).normalized();
        if (std::isnan(result.normal.x()) || result.normal.z() < 0)
            result.normal = Eigen::Vector3f(0.0f, 0.0f, 1.0f);
    } else {
        result.normal = lamb_n;
    }

    // --- Step 4: Shadow detection (non-metallic) ---
    // Samples well below the Lambertian prediction are likely in shadow.
    // Shadow probability = how far below the prediction, normalized by prediction.
    //float shadow_sigma = std::sqrt(result.lambertian_variance) + 1e-6f;
    for (size_t j = 0; j < K; j++) {
        if(result.is_metallic) {
            result.shadow_probability[j] = 0.0f;
            continue;
        }
        float ndotl = std::max(result.normal.dot(L[j]), 0.0f);
        if (ndotl < 0.01f) {
            // Light behind the surface — definitely shadowed
            result.shadow_probability[j] = 1.0f;
            continue;
        }
        float expected = lamb_albedo * ndotl;
        float deficit = expected - lum[j];
        if (deficit > 0.0f) {
            // How many sigma below expected?
            float zscore = deficit / expected;
            result.shadow_probability[j] = zscore;
        }

    }

    // TODO: Shadows usually form a contiguous region in light-direction space
    // (e.g. all lights from one side are blocked). Analyze the spatial coherence
    // of high-probability shadow samples on the light-direction hemisphere.
    // Cluster them and reject isolated outliers that are more likely noise or
    // specular highlights rather than true cast shadows.

    return result;
}

} // namespace brdf
