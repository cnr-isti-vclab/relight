#ifndef BRDF_MATH_H
#define BRDF_MATH_H

#include <vector>
#include <Eigen/Dense>

namespace brdf {

// Computes the Lambertian photometric stereo for a single pixel.
// I: Vector of K intensities (one for each light)
// L: Kx3 matrix of light directions (each row is [lx, ly, lz])
// Returns a 3D vector which is the unnormalized normal (the norm represents the albedo).
Eigen::Vector3f simple_lambertian_photometric_stereo(const Eigen::VectorXf& I, const Eigen::MatrixXf& L);

// Evaluates the GGX microfacet BRDF for a single light direction and a single color channel.
// Templated so it can be used with both standard floats and Ceres' AutoDiff Jet types.
template <typename T>
Eigen::Matrix<T, 3, 1> eval_ggx(
		const Eigen::Matrix<T, 3, 1>& N,
		const Eigen::Matrix<T, 3, 1>& L,
		const Eigen::Matrix<T, 3, 1>& V,
		const Eigen::Matrix<T, 3, 1>& albedo,
		const T& roughness,
		const Eigen::Matrix<T, 3, 1>& specular,
		const T& light_intensity)
{
	using std::sqrt;

	T rough = roughness < T(0.02) ? T(0.02) : roughness;
	T alpha = rough * rough;
	T alpha2 = alpha * alpha;

	Eigen::Matrix<T, 3, 1> H = (L + V).normalized();

	T NdotL = N.dot(L); NdotL = NdotL < T(0) ? T(0) : NdotL;
	T NdotV = N.dot(V); NdotV = NdotV < T(1e-4) ? T(1e-4) : NdotV;
	T NdotH = N.dot(H); NdotH = NdotH < T(1e-4) ? T(1e-4) : NdotH;
	T VdotH = V.dot(H); VdotH = VdotH < T(1e-4) ? T(1e-4) : VdotH;

	// GGX Distribution (D)
	T denom = (NdotH * NdotH) * (alpha2 - T(1.0)) + T(1.0);
	T D = alpha2 / (T(M_PI) * denom * denom + T(1e-7));

	// Smith Geometric Shadowing (G)
	T gv = NdotV + sqrt(alpha2 + (T(1.0) - alpha2) * NdotV * NdotV + T(1e-7));
	T gl = NdotL + sqrt(alpha2 + (T(1.0) - alpha2) * NdotL * NdotL + T(1e-7));
	T G = T(1.0) / (gv * gl + T(1e-7));

	// Fresnel Schlick (F)
	T vh_term = T(1.0) - VdotH;
	vh_term = vh_term < T(0) ? T(0) : vh_term;
	T vh5 = vh_term * vh_term * vh_term * vh_term * vh_term;
	Eigen::Matrix<T, 3, 1> F = specular + (Eigen::Matrix<T, 3, 1>::Ones() - specular) * vh5;

	// Height-correlated Smith G / (4 * NdotL * NdotV)
	T lambdaV = NdotL * sqrt((-NdotV * alpha2 + NdotV) * NdotV + alpha2);
	T lambdaL = NdotV * sqrt((-NdotL * alpha2 + NdotL) * NdotL + alpha2);
	T V_vis = T(0.5) / (lambdaV + lambdaL + T(1e-7));

	// Specular result
	Eigen::Matrix<T, 3, 1> spec = D * V_vis * F;

	// Combine Specular
	//Eigen::Matrix<T, 3, 1> spec = (D * G * F) / (T(4.0) * NdotV * NdotL + T(1e-6));

	// Diffuse + Ambient
	Eigen::Matrix<T, 3, 1> diff = (Eigen::Matrix<T, 3, 1>::Ones() - F).cwiseProduct(albedo) / T(M_PI);
	Eigen::Matrix<T, 3, 1> ambient = T(0.01) * albedo;

	Eigen::Matrix<T, 3, 1> shade = light_intensity * (diff * NdotL + spec * NdotL + ambient * NdotL);

	// Ensure no negative values due to precision issues
	shade(0) = shade(0) < T(0) ? T(0) : shade(0);
	shade(1) = shade(1) < T(0) ? T(0) : shade(1);
	shade(2) = shade(2) < T(0) ? T(0) : shade(2);

	return shade;
}

struct BRDFSampleRGB {
	double theta_h;           // Angle between N e H
	Eigen::Vector3d rgb_val;
};

std::vector<BRDFSampleRGB> ExtractBRDFProfileRGB(
		const Eigen::Vector3d& normal,
		const std::vector<Eigen::Vector3d>& light_dirs,
		const std::vector<Eigen::Vector3d>& rgb_samples)
{
	std::vector<BRDFSampleRGB> profile;
	Eigen::Vector3d V(0, 0, 1);

	for (size_t i = 0; i < light_dirs.size(); ++i) {
		Eigen::Vector3d L = light_dirs[i];
		double n_dot_l = normal.dot(L);

		// Shadow Masking
		if (n_dot_l < 0.05) continue;

		Eigen::Vector3d H = (L + V).normalized();
		double cos_theta_h = std::clamp(normal.dot(H), 0.0, 1.0);
		double theta_h = std::acos(cos_theta_h);

		// Divide by n_dot_l to take Lambertian into account
		Eigen::Vector3d brdf_rgb = rgb_samples[i] / (n_dot_l + 1e-4);

		profile.push_back({theta_h, brdf_rgb});
	}

	std::sort(profile.begin(), profile.end(), [](const BRDFSampleRGB& a, const BRDFSampleRGB& b) {
		return a.theta_h < b.theta_h;
	});

	return profile;
}

double ComputeMaterialSimilarityRGB(const std::vector<BRDFSampleRGB>& p1, const std::vector<BRDFSampleRGB>& p2) {
	if (p1.empty() || p2.empty()) return 0.0;

	const int num_bins = 18;
	std::vector<Eigen::Vector3d> bin_sums1(num_bins, Eigen::Vector3d::Zero());
	std::vector<Eigen::Vector3d> bin_sums2(num_bins, Eigen::Vector3d::Zero());
	std::vector<int> bin_counts1(num_bins, 0), bin_counts2(num_bins, 0);

	auto fill_bins = [&](const std::vector<BRDFSampleRGB>& p, std::vector<Eigen::Vector3d>& sums, std::vector<int>& counts) {
		for (const auto& s : p) {
			int bin = std::clamp((int)(s.theta_h * (180.0 / M_PI) / 5.0), 0, num_bins - 1);
			sums[bin] += s.rgb_val;
			counts[bin]++;
		}
	};

	fill_bins(p1, bin_sums1, bin_counts1);
	fill_bins(p2, bin_sums2, bin_counts2);

	double total_l2_dist = 0.0;
	int valid_bins = 0;

	for (int i = 0; i < num_bins; ++i) {
		if (bin_counts1[i] > 0 && bin_counts2[i] > 0) {
			Eigen::Vector3d mean1 = bin_sums1[i] / (double)bin_counts1[i];
			Eigen::Vector3d mean2 = bin_sums2[i] / (double)bin_counts2[i];

			total_l2_dist += (mean1 - mean2).squaredNorm();
			valid_bins++;
		}
	}

	if (valid_bins == 0) return 0.0;

	double rms_error = std::sqrt(total_l2_dist / (valid_bins * 3.0));

	//TODO: Range [0, 1] hopefully we might want to change the weighting function
	return std::exp(-rms_error * 5.0);
}

// TODO: analyze_specularity

} // namespace brdf

#endif // BRDF_MATH_H
