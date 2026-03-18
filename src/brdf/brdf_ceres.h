#ifndef BRDF_CERES_H
#define BRDF_CERES_H

#include "brdf_math.h"
#include <ceres/ceres.h>
#include <Eigen/Dense>

namespace brdf {

// Cost functor for Ceres solver to optimize GGX BRDF parameters per pixel.
// Each instance of this cost function represents the residual for ONE light direction.
struct GgxResidual {
	GgxResidual(const Eigen::Vector3f& light_dir,
				const Eigen::Vector3f& view_dir,
				const Eigen::Vector3f& observed_color,
				float light_intensity)
		: L_(light_dir), V_(view_dir), observed_color_(observed_color),
		  light_intensity_(light_intensity) {}

	template <typename T>
	bool operator()(const T* const normal,
					const T* const roughness,
					const T* const specular,
					const T* const albedo,
					T* residual) const {

		// Construct Eigen vectors using the generic type T (could be double or Jet for AutoDiff)
		Eigen::Matrix<T, 3, 1> N(normal[0], normal[1], normal[2]);
		N.normalize(); // Ensure the normal stays spherical

		Eigen::Matrix<T, 3, 1> L(T(L_.x()), T(L_.y()), T(L_.z()));
		Eigen::Matrix<T, 3, 1> V(T(V_.x()), T(V_.y()), T(V_.z()));
		Eigen::Matrix<T, 3, 1> albedo_vec(albedo[0], albedo[1], albedo[2]);
		Eigen::Matrix<T, 3, 1> specular_vec(specular[0], specular[1], specular[2]);

		T intensity(light_intensity_);

		// Evaluate the BRDF mathematically
		Eigen::Matrix<T, 3, 1> pred_color = eval_ggx(N, L, V, albedo_vec, roughness[0], specular_vec, intensity);

		// Compute residuals for RGB
		residual[0] = pred_color(0) - T(observed_color_.x());
		residual[1] = pred_color(1) - T(observed_color_.y());
		residual[2] = pred_color(2) - T(observed_color_.z());

		return true;
	}

	// Factory to hide the boilerplate of cost function creation
	static ceres::CostFunction* Create(const Eigen::Vector3f& light_dir,
									   const Eigen::Vector3f& view_dir,
									   const Eigen::Vector3f& observed_color,
									   float light_intensity) {
		return (new ceres::AutoDiffCostFunction<GgxResidual, 3, 3, 1, 3, 3>(
					new GgxResidual(light_dir, view_dir, observed_color, light_intensity)));
	}

private:
	Eigen::Vector3f L_;
	Eigen::Vector3f V_;
	Eigen::Vector3f observed_color_;
	float light_intensity_;
};

} // namespace brdf

#endif // BRDF_CERES_H
