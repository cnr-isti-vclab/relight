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

} // namespace brdf
