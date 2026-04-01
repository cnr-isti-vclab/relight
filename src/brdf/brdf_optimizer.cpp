#include "brdf_optimizer.h"
#include "brdf_ceres.h"
#include <ceres/ceres.h>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace brdf {

BrdfFitResult optimize_brdf_pixel(
		const Pixel& I,
		const std::vector<Eigen::Vector3f>& L,
		const Eigen::Vector3f& initial_normal,
		const Eigen::Vector3f& initial_albedo,
		float initial_roughness,
        float initial_metallic,
		float light_intensity,
		bool optimize_normal,
		bool optimize_albedo)
{
	// The variables to be optimized
	double normal[3] = { initial_normal.x(), initial_normal.y(), initial_normal.z() };
	double roughness[1] = { static_cast<double>(initial_roughness) };
	double albedo[3] = { static_cast<double>(initial_albedo.x()), static_cast<double>(initial_albedo.y()), static_cast<double>(initial_albedo.z()) };

	// Metallic starts at 0 (dielectric). The optimizer will push it toward 1 for metallic surfaces.
    double metallic[1] = { initial_metallic };

	//TODO this is not entirely true!
	// View direction (Orthographic assumed from Dome, looking down Z)
	Eigen::Vector3f V(0.0f, 0.0f, 1.0f);

	ceres::Problem problem;

	for(size_t k = 0; k < I.size(); ++k) {
		Eigen::Vector3f observed_color(I[k].r, I[k].g, I[k].b);

        //ceres::CostFunction* cost_function = GgxResidual::Create(L[k], V, observed_color, light_intensity);
        ceres::CostFunction* cost_function = GltfResidual::Create(L[k], V, observed_color, Eigen::Vector3f(light_intensity, light_intensity, light_intensity));

		problem.AddResidualBlock(cost_function, nullptr, normal, roughness, metallic, albedo);
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

	// Metallic bounds [0, 1]
	problem.SetParameterLowerBound(metallic, 0, 0.0);
	problem.SetParameterUpperBound(metallic, 0, 1.0);

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
	result.metallic = metallic[0];
	result.albedo = Eigen::Vector3f(albedo[0], albedo[1], albedo[2]);

	return result;
}

BrdfFitResult optimize_brdf_patch_material(
        const std::vector<Pixel>&           pixels,
        const std::vector<Eigen::Vector3f>& normals,
        const std::vector<Eigen::Vector3f>& L,
        const Eigen::Vector3f& initial_albedo,
        float initial_roughness,
        float initial_metallic,
        float light_intensity)
{
	const Eigen::Vector3f V(0.0f, 0.0f, 1.0f);
	const Eigen::Vector3f lightColor(light_intensity, light_intensity, light_intensity);

	double roughness[1] = { double(initial_roughness) };
	double metallic[1]  = { double(initial_metallic)  };
	double albedo[3]    = { double(initial_albedo.x()),
	                        double(initial_albedo.y()),
	                        double(initial_albedo.z()) };

	ceres::Problem problem;

	const int P = int(pixels.size());
	for (int p = 0; p < P; ++p) {
		const Pixel& px       = pixels[p];
		const Eigen::Vector3f& N = (p < int(normals.size())) ? normals[p] : Eigen::Vector3f(0, 0, 1);
		for (int k = 0; k < int(px.size()) && k < int(L.size()); ++k) {
			Eigen::Vector3f obs(px[k].r, px[k].g, px[k].b);
			problem.AddResidualBlock(
				GltfResidualFixedNormal::Create(N, L[k], V, obs, lightColor),
				nullptr,
				roughness, metallic, albedo);
		}
	}

	problem.SetParameterLowerBound(roughness, 0, 0.02);
	problem.SetParameterUpperBound(roughness, 0, 1.0);
	problem.SetParameterLowerBound(metallic,  0, 0.0);
	problem.SetParameterUpperBound(metallic,  0, 1.0);
	for (int c = 0; c < 3; ++c) {
		problem.SetParameterLowerBound(albedo, c, 0.0);
		problem.SetParameterUpperBound(albedo, c, 1.0);
	}

	ceres::Solver::Options options;
	options.max_num_iterations          = 200;
	options.linear_solver_type          = ceres::DENSE_QR;
	options.minimizer_progress_to_stdout = false;

	ceres::Solver::Summary summary;
	ceres::Solve(options, &problem, &summary);

	// Return a zero normal — caller already owns the per-pixel normals.
	BrdfFitResult result;
	result.normal    = Eigen::Vector3f(0.0f, 0.0f, 1.0f);
	result.roughness = float(roughness[0]);
	result.metallic  = float(metallic[0]);
	result.albedo    = Eigen::Vector3f(float(albedo[0]), float(albedo[1]), float(albedo[2]));
	return result;
}

// Compute sum-of-squared-errors between BRDF prediction and observed pixel across all lights.
float compute_pixel_sse(
		const Pixel& I,
		const std::vector<Eigen::Vector3f>& L,
		const Eigen::Vector3f& N,
		const Eigen::Vector3f& albedo,
		float roughness,
        float metallic,
		float light_intensity)
{
	static const Eigen::Vector3f V(0.0f, 0.0f, 1.0f);
	float sse = 0.0f;
	for(size_t k = 0; k < I.size(); ++k) {
        //Eigen::Vector3f pred = eval_ggx(N, L[k], V, albedo, roughness, specular, light_intensity);
        Eigen::Vector3f light(light_intensity, light_intensity, light_intensity);
        Eigen::Vector3f pred = eval_gltf(N, L[k], V, albedo, metallic, roughness, light);
		float dr = pred.x() - I[k].r;
		float dg = pred.y() - I[k].g;
		float db = pred.z() - I[k].b;
		sse += dr*dr + dg*dg + db*db;
	}
	return sse;
}


void brdf_bruteforce_compare(
		const Pixel& I,
		const std::vector<Eigen::Vector3f>& L,
		const BrdfFitResult& ceres_result,
		float light_intensity,
		int pixel_x, int pixel_y,
		std::ostream& out,
		float step,
		bool bruteforce_normal,
		bool bruteforce_albedo)
{
	struct Candidate {
		float sse;
		Eigen::Vector3f normal;
		Eigen::Vector3f albedo;
		float roughness;
        float metallic;
	};

	int nsteps = (int)std::round(1.0f / step);

	// Build the list of normals to try (upper hemisphere, Cartesian grid)
	std::vector<Eigen::Vector3f> normals_to_try;
	if(bruteforce_normal) {
		for(int nxi = -nsteps; nxi <= nsteps; ++nxi) {
			for(int nyi = -nsteps; nyi <= nsteps; ++nyi) {
				float nx = nxi * step;
				float ny = nyi * step;
				float nz2 = 1.0f - nx*nx - ny*ny;
				if(nz2 <= 0.0f) continue;
				normals_to_try.push_back(Eigen::Vector3f(nx, ny, std::sqrt(nz2)));
			}
		}
	} else {
		normals_to_try.push_back(ceres_result.normal);
	}

	// Build the list of albedos to try (full RGB, 3D)
	std::vector<Eigen::Vector3f> albedos_to_try;
	if(bruteforce_albedo) {
		for(int ar = 0; ar <= nsteps; ++ar)
			for(int ag = 0; ag <= nsteps; ++ag)
				for(int ab = 0; ab <= nsteps; ++ab)
					albedos_to_try.push_back(Eigen::Vector3f(ar * step, ag * step, ab * step));
	} else {
		albedos_to_try.push_back(ceres_result.albedo);
	}

	// Max-heap keeping the 10 smallest SSE candidates (largest at heap.front()).
	std::vector<Candidate> heap;
	heap.reserve(11);
	auto cmp = [](const Candidate& a, const Candidate& b) { return a.sse < b.sse; };

	for(const auto& N_ : normals_to_try) {
		for(const auto& A_ : albedos_to_try) {
			for(int ri = 1; ri <= nsteps; ++ri) {
				float r = ri * step;
                for(int mm = 0; mm <= nsteps; ++mm) {
                    float m = mm * step;
                    float sse = compute_pixel_sse(I, L, N_, A_, r, m, light_intensity);
                    Candidate c{sse, N_, A_, r, m};
					if((int)heap.size() < 10) {
						heap.push_back(c);
						if((int)heap.size() == 10)
							std::make_heap(heap.begin(), heap.end(), cmp);
					} else if(sse < heap.front().sse) {
						std::pop_heap(heap.begin(), heap.end(), cmp);
						heap.back() = c;
						std::push_heap(heap.begin(), heap.end(), cmp);
					}
				}
			}
		}
	}

	std::sort(heap.begin(), heap.end(),
		[](const Candidate& a, const Candidate& b) { return a.sse < b.sse; });

	float ceres_sse = compute_pixel_sse(I, L, ceres_result.normal, ceres_result.albedo,
        ceres_result.roughness, ceres_result.metallic, light_intensity);

	out << std::fixed << std::setprecision(4);
	out << "=== Pixel (" << pixel_x << ", " << pixel_y << ")  nlights=" << I.size() << " ===\n";
	out << "\tCeres  : normal=[" << ceres_result.normal.x() << ", " << ceres_result.normal.y() << ", " << ceres_result.normal.z() << "]"
		<< "  albedo=[" << ceres_result.albedo.x() << ", " << ceres_result.albedo.y() << ", " << ceres_result.albedo.z() << "]"
		<< "  rough=" << ceres_result.roughness
		<< "  metallic=" << ceres_result.metallic
		<< "  SSE=" << ceres_sse << "\n";
    out << "\tBrute-force top-10 (step=" << step << ", roughness x metallic"
		<< (bruteforce_normal ? " x normal" : "")
		<< (bruteforce_albedo ? " x albedo_rgb" : "")
		<< "):\n";
	for(int k = 0; k < (int)heap.size(); ++k) {
		out << "\t\t" << (k+1) << ". ";
		if(bruteforce_normal)
			out << "normal=[" << heap[k].normal.x() << ", " << heap[k].normal.y() << ", " << heap[k].normal.z() << "]  ";
		if(bruteforce_albedo)
			out << "albedo=[" << heap[k].albedo.x() << ", " << heap[k].albedo.y() << ", " << heap[k].albedo.z() << "]  ";
		out << "rough=" << heap[k].roughness
            << "  metalilc=" << heap[k].metallic
			<< "  SSE=" << heap[k].sse << "\n";
	}
	out << "\n";
}



} // namespace brdf
