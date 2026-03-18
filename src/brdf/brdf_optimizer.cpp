#include "brdf_optimizer.h"
#include "brdf_ceres.h"
#include <ceres/ceres.h>

namespace brdf {

BrdfFitResult optimize_brdf_pixel(
		const Pixel& I,
		const std::vector<Eigen::Vector3f>& L,
		const Eigen::Vector3f& initial_normal,
		const Eigen::Vector3f& initial_albedo,
		float initial_roughness,
		float light_intensity,
		bool optimize_normal,
		bool optimize_albedo)
{
	// The variables to be optimized
	double normal[3] = { initial_normal.x(), initial_normal.y(), initial_normal.z() };
	double roughness[1] = { static_cast<double>(initial_roughness) };
	double albedo[3] = { static_cast<double>(initial_albedo.x()), static_cast<double>(initial_albedo.y()), static_cast<double>(initial_albedo.z()) };

	// Specular color is typical initialized to dielectric default (0.04) or using specular heuristics
	// python script used: 0.04 + 0.4 * spec_mask
	double specular[3] = { 0.08, 0.08, 0.08 };

	//TODO this is not entirely true!
	// View direction (Orthographic assumed from Dome, looking down Z)
	Eigen::Vector3f V(0.0f, 0.0f, 1.0f);

	ceres::Problem problem;

	for (size_t k = 0; k < I.size(); ++k) {
		Eigen::Vector3f observed_color(I[k].r, I[k].g, I[k].b);

		ceres::CostFunction* cost_function =
				GgxResidual::Create(L[k], V, observed_color, light_intensity);

		// Add the residual block
		// We use a Huber loss (SoftL1) function for robustness against highlights/shadows
		ceres::LossFunction* loss_function = new ceres::HuberLoss(0.1);

		problem.AddResidualBlock(cost_function, loss_function, normal, roughness, specular, albedo);
	}

	// Make normal constant if not optimizing it
	if (!optimize_normal) {
		problem.SetParameterBlockConstant(normal);
	}

	// Make albedo constant if not optimizing it
	if (!optimize_albedo) {
		problem.SetParameterBlockConstant(albedo);
	}

	// Set bounds to ensure physically plausible values
	// Roughness bounds
	problem.SetParameterLowerBound(roughness, 0, 0.02);
	problem.SetParameterUpperBound(roughness, 0, 1.0);

	// Specular color bounds
	problem.SetParameterLowerBound(specular, 0, 0.0);
	problem.SetParameterUpperBound(specular, 0, 1.0);
	problem.SetParameterLowerBound(specular, 1, 0.0);
	problem.SetParameterUpperBound(specular, 1, 1.0);
	problem.SetParameterLowerBound(specular, 2, 0.0);
	problem.SetParameterUpperBound(specular, 2, 1.0);

	// Albedo bounds (only set if optimizing)
	if (optimize_albedo) {
		problem.SetParameterLowerBound(albedo, 0, 0.0);
		problem.SetParameterUpperBound(albedo, 0, 1.0);
		problem.SetParameterLowerBound(albedo, 1, 0.0);
		problem.SetParameterUpperBound(albedo, 1, 1.0);
		problem.SetParameterLowerBound(albedo, 2, 0.0);
		problem.SetParameterUpperBound(albedo, 2, 1.0);
	}

	// Optionally parameterize normal to stay on the unit sphere
	// We constrain the vector but keeping the 3 degrees of freedom + normalization
	// inside the GgxResidual is often mathematically enough. Alternatively we could
	// use a ceres::Manifold (ceres::LocalParameterizer in older APIs) to strictly move on S2.
	// For now we do implicit normalization in the Cost Functor (N.normalize()).

	// Solve configuration
	ceres::Solver::Options options;
	options.max_num_iterations = 200;
	options.linear_solver_type = ceres::DENSE_QR;
	options.minimizer_progress_to_stdout = false;

	ceres::Solver::Summary summary;
	ceres::Solve(options, &problem, &summary);

	BrdfFitResult result;
	Eigen::Vector3f optimized_normal(normal[0], normal[1], normal[2]);
	optimized_normal.normalize();

	result.normal = optimized_normal;
	result.roughness = roughness[0];
	result.specular = Eigen::Vector3f(specular[0], specular[1], specular[2]);
	result.albedo = Eigen::Vector3f(albedo[0], albedo[1], albedo[2]);

	return result;
}

} // namespace brdf
