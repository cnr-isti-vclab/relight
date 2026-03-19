#include "brdf_math.h"
#include <cmath>
#include <algorithm>

namespace brdf {

Eigen::Vector3f simple_lambertian_photometric_stereo(const Eigen::VectorXf& I, const Eigen::MatrixXf& L) {
    Eigen::Matrix3f LtL = L.transpose() * L;
    LtL += 1e-5f * Eigen::Matrix3f::Identity();
    
    Eigen::Vector3f LtI = L.transpose() * I;
    
    Eigen::Vector3f m = LtL.ldlt().solve(LtI);
    
    if (m(2) < 0.0f) {
        m = -m;
    }
    
    return m;
}


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

} // namespace brdf
