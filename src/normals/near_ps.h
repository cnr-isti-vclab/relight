#ifndef NEAR_PS_H
#define NEAR_PS_H

#include <Eigen/Core>
#include <Eigen/Sparse>
#include <vector>
#include <functional>
#include <string>

/*
 * Near-light Photometric Stereo
 *
 * C++ implementation of the method described in:
 * [1] "LED-based Photometric Stereo: Modeling, Calibration and Numerical Solution"
 *      Quéau et al., 2017
 * [2] "Semi-calibrated Near-Light Photometric Stereo"
 *      Quéau et al., Proc. SSVM 2017
 *
 * Solves photometric stereo under nearby point light sources, jointly estimating
 * depth (XYZ), surface normals, albedo and optionally light intensities.
 */

/// Robust M-estimator type for the data term
enum class NearPSEstimator {
	LS,      ///< Least Squares (no robustness)
	Cauchy,  ///< Cauchy's M-estimator
	Lp,      ///< Lp-norm estimator
	GM,      ///< Geman-McClure M-estimator
	Welsh,   ///< Welsh M-estimator
	Tukey    ///< Tukey's biweight M-estimator
};

/// Preconditioner for the inner PCG solver
enum class NearPSPrecond {
	Jacobi,  ///< Diagonal (Jacobi) preconditioner
	ICholmod ///< Incomplete Cholesky
};

/// Parameters for the near-light photometric stereo solver
struct NearPSParams {
	/// Initial depth value (constant across the image) or per-pixel z0.
	/// If z0 is empty, default_z0 is used as a constant.
	double default_z0 = 700.0;
	std::vector<double> z0;

	/// Semi-calibrated mode: if true, light intensities (Phi) are refined during iteration
	bool semi_calibrated = false;

	/// Robust M-estimator
	NearPSEstimator estimator = NearPSEstimator::LS;

	/// Parameter for the robust estimator (0 for LS; recommend ~0.1 for Cauchy)
	double lambda = 0.0;

	/// Fidelity to initial depth z0 (0 = no regularization)
	double zeta = 0.0;

	/// Whether to model self-shadows with max(shading, 0)
	bool self_shadows = true;

	/// Max outer iterations
	int maxit = 100;

	/// Relative energy stopping criterion
	double tol = 1e-3;

	/// Stopping criterion: median angular change in normals (degrees)
	double tol_normals = 0.1;

	/// Integer downsampling factor (1 = no downsampling)
	int ratio = 1;

	/// Number of scales for coarse-to-fine (1 = single scale)
	int scales = 1;

	/// PCG solver tolerance
	double tol_pcg = 1e-6;

	/// PCG solver max iterations
	int maxit_pcg = 25;

	/// Preconditioner
	NearPSPrecond precond = NearPSPrecond::Jacobi;
};

/// Calibration data: light source positions, camera intrinsics, etc.
struct NearPSCalib {
	/// NIMGS x 3 light source positions (one row per image)
	Eigen::MatrixXd S;

	/// 3x3 camera intrinsic matrix
	Eigen::Matrix3d K;

	/// NIMGS x NCHANNELS light intensities (default: all ones)
	Eigen::MatrixXd Phi;

	/// NIMGS x 1 anisotropy factors (default: all zeros, isotropic)
	Eigen::VectorXd mu;

	/// NIMGS x 3 light orientations (default: [0,0,1] for each)
	Eigen::MatrixXd Dir;
};

/// Input image data
struct NearPSData {
	/// Images: nrows x ncols x nimgs (grayscale) stored as a flat vector
	/// For color: nrows x ncols x nchannels x nimgs
	/// Layout: pixel (row, col, img) = I[img * nrows*ncols + row*ncols + col]  (grayscale)
	///         pixel (row, col, ch, img) = I[img*nrows*ncols*nchannels + ch*nrows*ncols + row*ncols + col] (color)
	std::vector<double> I;
	int nrows = 0;
	int ncols = 0;
	int nimgs = 0;
	int nchannels = 1;  // 1 for grayscale, 3 for color

	/// Optional binary mask (nrows x ncols), true = use pixel.
	/// If empty, all pixels are used.
	std::vector<uint8_t> mask;

	/// Optional per-pixel usage mask (same size as I).
	/// If empty, all data is used.
	std::vector<uint8_t> used;
};

/// Output results from near-light PS
struct NearPSResult {
	/// nrows x ncols depth map
	std::vector<double> Z;

	/// nrows x ncols x 3 normal map (Nx, Ny, Nz interleaved per pixel)
	std::vector<Eigen::Vector3d> N;

	/// nrows x ncols x nchannels albedo map
	std::vector<double> rho;

	/// nimgs x nchannels refined light intensities
	Eigen::MatrixXd Phi;

	/// Binary mask used
	std::vector<uint8_t> mask;

	/// Energy at each iteration
	std::vector<double> tab_nrj;

	int nrows = 0;
	int ncols = 0;
	int nchannels = 1;
};

/// Run the near-light photometric stereo algorithm.
///
/// @param data      Input images and mask
/// @param calib     Light source positions, camera intrinsics, etc.
/// @param params    Algorithm parameters
/// @param progress  Optional callback: progress(message, percent) returns false to cancel.
/// @return          NearPSResult with depth, normals, albedo, etc.
NearPSResult near_ps(const NearPSData &data,
                     const NearPSCalib &calib,
                     const NearPSParams &params = NearPSParams(),
                     std::function<bool(std::string, int)> progress = nullptr);

#endif // NEAR_PS_H
