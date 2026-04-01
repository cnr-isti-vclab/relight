#ifndef BRDF_BIVARIATE_H
#define BRDF_BIVARIATE_H

#include <Eigen/Dense>
#include <vector>
#include <cmath>

#include "ceres/ceres.h"

// ---------------------------------------------------------------------------
// Shared-material patch normal initializer
//
// All P pixels in a patch share the same BRDF (a mixture of J GGX lobes with
// fixed roughness values alpha[j]).  Each pixel has its own normal.  The
// optimizer jointly solves for:
//   - P normals  (one 3-vector each, constrained to the unit sphere)
//   - J weights  (shared across all pixels, non-negative)
//
// The view direction is assumed to be (0,0,1) – overhead, standard for RTI.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Cost: one observation (pixel p, light k, single luminance channel)
// DynamicAutoDiffCostFunction interface: params[0]=normal(3), params[1]=weights(J)
// ---------------------------------------------------------------------------
struct SharedMaterialResidual {
    SharedMaterialResidual(const Eigen::Vector3d& light,
                           double obs_intensity,
                           const std::vector<double>& alphas)
        : light_(light), obs_(obs_intensity), alphas_(alphas) {}

    template <typename T>
    bool operator()(T const* const* params, T* residual) const {
        const T* normal  = params[0];
        const T* weights = params[1];

        Eigen::Matrix<T, 3, 1> n(normal[0], normal[1], normal[2]);
        const Eigen::Matrix<T, 3, 1> v(T(0), T(0), T(1));
        Eigen::Matrix<T, 3, 1> l = light_.cast<T>();
        Eigen::Matrix<T, 3, 1> h = (l + v).normalized();

        T NdotL = n.dot(l);
        NdotL = NdotL < T(0) ? T(0) : NdotL;
        T NdotH = n.dot(h);
        NdotH = NdotH < T(1e-6) ? T(1e-6) : NdotH;

        T predicted = T(0);
        for (int j = 0; j < int(alphas_.size()); ++j) {
            T a2    = T(alphas_[j] * alphas_[j]);
            T cos2  = NdotH * NdotH;
            T denom = cos2 * (a2 - T(1)) + T(1);
            T D     = a2 / (T(M_PI) * denom * denom);
            predicted += weights[j] * D * NdotL;
        }

        residual[0] = predicted - T(obs_);
        return true;
    }

    static ceres::CostFunction* Create(const Eigen::Vector3d& light,
                                       double obs,
                                       const std::vector<double>& alphas) {
        auto* cost = new ceres::DynamicAutoDiffCostFunction<SharedMaterialResidual>(
            new SharedMaterialResidual(light, obs, alphas));
        cost->AddParameterBlock(3);
        cost->AddParameterBlock(int(alphas.size()));
        cost->SetNumResiduals(1);
        return cost;
    }

private:
    const Eigen::Vector3d      light_;
    const double               obs_;
    const std::vector<double>  alphas_;
};

// ---------------------------------------------------------------------------
// Soft monotonicity penalty: encourages w[j] >= w[j+1]
// DynamicAutoDiffCostFunction interface: params[0]=weights(J)
// ---------------------------------------------------------------------------
struct MonotonicityPenalty {
    explicit MonotonicityPenalty(int index) : j(index) {}

    template <typename T>
    bool operator()(T const* const* params, T* residual) const {
        const T* weights = params[0];
        T diff = weights[j + 1] - weights[j];
        residual[0] = (diff > T(0)) ? diff * T(100.0) : T(0);
        return true;
    }

    static ceres::CostFunction* Create(int index, int J) {
        auto* cost = new ceres::DynamicAutoDiffCostFunction<MonotonicityPenalty>(
            new MonotonicityPenalty(index));
        cost->AddParameterBlock(J);
        cost->SetNumResiduals(1);
        return cost;
    }

    int j;
};

// ---------------------------------------------------------------------------
// SolvePatch
//
// lights[k]           : K light directions (unit 3-vectors), same for all pixels
// observations[p*K+k] : luminance for pixel p under light k
// alphas              : J fixed roughness values for the GGX basis
// patch_normals       : [in/out] P 3-vectors (warm-started, updated in place)
// shared_weights      : [in/out] J weights (warm-started, updated in place)
// ---------------------------------------------------------------------------
inline void SolvePatch(const std::vector<Eigen::Vector3d>& lights,   // size K
                       const std::vector<double>&           observations, // size P*K
                       const std::vector<double>&           alphas,
                       std::vector<Eigen::Vector3d>&        patch_normals, // size P
                       std::vector<double>&                 shared_weights) // size J
{
    const int K = int(lights.size());
    const int P = int(patch_normals.size());
    const int J = int(alphas.size());

    ceres::Problem problem;

    // ---- Shared material weights (non-negative) ---------------------------
    problem.AddParameterBlock(shared_weights.data(), J);
    for (int j = 0; j < J; ++j)
        problem.SetParameterLowerBound(shared_weights.data(), j, 0.0);

    // ---- Per-pixel normals (on unit sphere) + residual blocks ------------
    for (int p = 0; p < P; ++p) {
        double* n_ptr = patch_normals[p].data();
        problem.AddParameterBlock(n_ptr, 3);
        problem.SetManifold(n_ptr, new ceres::SphereManifold<3>);

        for (int k = 0; k < K; ++k) {
            int idx = p * K + k;
            problem.AddResidualBlock(
                SharedMaterialResidual::Create(lights[k], observations[idx], alphas),
                new ceres::HuberLoss(0.05),
                n_ptr,
                shared_weights.data());
        }
    }

    // ---- Monotonicity regularization on weights --------------------------
    for (int j = 0; j < J - 1; ++j) {
        problem.AddResidualBlock(
            MonotonicityPenalty::Create(j, J),
            nullptr,
            shared_weights.data());
    }

    // ---- Solve -----------------------------------------------------------
    ceres::Solver::Options options;
    options.linear_solver_type        = ceres::DENSE_SCHUR;
    options.max_num_iterations        = 100;
    options.minimizer_progress_to_stdout = false;

    ceres::Solver::Summary summary;
    ceres::Solve(options, &problem, &summary);
    // On failure the warm-start values are left unchanged.
}

#endif // BRDF_BIVARIATE_H