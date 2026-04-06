#include "near_ps.h"

#include <Eigen/Dense>
#include <Eigen/SparseCore>
#include <Eigen/IterativeLinearSolvers>
#include <Eigen/SparseCholesky>

#include <cmath>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <cassert>

using namespace Eigen;
using namespace std;

// ============================================================================
// Utility: bilinear resize for a flat double array
// ============================================================================

static void resizeImage(const double *input, int iw, int ih,
                        double *output, int ow, int oh) {
	if (ow <= 0 || oh <= 0) return;
	float x_ratio = (ow > 1) ? (float(iw) - 1.0f) / (float(ow) - 1.0f) : 0.0f;
	float y_ratio = (oh > 1) ? (float(ih) - 1.0f) / (float(oh) - 1.0f) : 0.0f;

	for (int i = 0; i < oh; i++) {
		for (int j = 0; j < ow; j++) {
			float fy = y_ratio * i;
			float fx = x_ratio * j;
			int x0 = int(fx), y0 = int(fy);
			int x1 = min(x0 + 1, iw - 1);
			int y1 = min(y0 + 1, ih - 1);
			float wx = fx - x0, wy = fy - y0;
			double a = input[y0 * iw + x0];
			double b = input[y0 * iw + x1];
			double c = input[y1 * iw + x0];
			double d = input[y1 * iw + x1];
			output[i * ow + j] = a * (1 - wx) * (1 - wy) + b * wx * (1 - wy) +
			                     c * (1 - wx) * wy + d * wx * wy;
		}
	}
}

// Resize a mask (uint8_t), using nearest-neighbor
static void resizeMask(const uint8_t *input, int iw, int ih,
                       uint8_t *output, int ow, int oh) {
	if (ow <= 0 || oh <= 0) return;
	for (int i = 0; i < oh; i++) {
		for (int j = 0; j < ow; j++) {
			int sy = min(int(float(i * ih) / oh), ih - 1);
			int sx = min(int(float(j * iw) / ow), iw - 1);
			output[i * ow + j] = input[sy * iw + sx];
		}
	}
}

// ============================================================================
// M-estimator functions: phi(x, lambda) and weight w(x, lambda) = phi'(x)/x
// ============================================================================

static double phi_LS(double x, double /*lambda*/) { return 0.5 * x * x; }
static double w_LS(double /*x*/, double /*lambda*/) { return 1.0; }

static double phi_Cauchy(double x, double lambda) { return 0.5 * log(1.0 + x * x / (lambda * lambda)); }
static double w_Cauchy(double x, double lambda) { return 1.0 / (lambda * lambda + x * x); }

static double phi_GM(double x, double lambda) { return 0.5 * lambda * lambda * x * x / (x * x + lambda * lambda); }
static double w_GM(double x, double lambda) { double d = x * x + lambda * lambda; return (lambda * lambda * lambda * lambda) / (d * d); }

static double phi_Welsh(double x, double lambda) { return 0.5 * lambda * lambda * (1.0 - exp(-x * x / (lambda * lambda))); }
static double w_Welsh(double x, double lambda) { return exp(-x * x / (lambda * lambda)); }

static double phi_Tukey(double x, double lambda) {
	double ax = fabs(x);
	if (ax <= lambda) {
		double r = 1.0 - x * x / (lambda * lambda);
		return (1.0 / 6.0) * (1.0 - r * r * r) * lambda * lambda;
	}
	return lambda * lambda / 6.0;
}
static double w_Tukey(double x, double lambda) {
	if (fabs(x) <= lambda) {
		double r = 1.0 - x * x / (lambda * lambda);
		return r * r;
	}
	return 0.0;
}

static const double kLpThrNorm = 1e-2;
static double phi_Lp(double x, double lambda) {
	return pow(fabs(x), lambda) / (lambda * pow(kLpThrNorm, lambda - 2.0));
}
static double w_Lp(double x, double lambda) {
	double mx = max(kLpThrNorm, fabs(x));
	return lambda * pow(mx, lambda - 2.0) / (lambda * pow(kLpThrNorm, lambda - 2.0));
}

using PhiFcn = double(*)(double, double);
using WFcn = double(*)(double, double);

static void selectEstimator(NearPSEstimator est, PhiFcn &phi, WFcn &w) {
	switch (est) {
	case NearPSEstimator::Cauchy: phi = phi_Cauchy; w = w_Cauchy; break;
	case NearPSEstimator::GM:     phi = phi_GM;     w = w_GM;     break;
	case NearPSEstimator::Welsh:  phi = phi_Welsh;  w = w_Welsh;  break;
	case NearPSEstimator::Tukey:  phi = phi_Tukey;  w = w_Tukey;  break;
	case NearPSEstimator::Lp:     phi = phi_Lp;     w = w_Lp;     break;
	default:                      phi = phi_LS;      w = w_LS;     break;
	}
}

// ============================================================================
// Build gradient operators (Dx, Dy) on the masked domain.
// Mimics make_gradient from the MATLAB code: forward differences where a
// neighbor exists, backward differences otherwise.
// ============================================================================

struct GradientOps {
	SparseMatrix<double> Dx;
	SparseMatrix<double> Dy;
	int npix; // number of mask pixels
};

static GradientOps buildGradient(const vector<uint8_t> &mask, int nrows, int ncols) {
	// Index map: for each pixel in the mask, assign an index in [0, npix)
	vector<int> index_matrix(nrows * ncols, -1);
	vector<int> imask;
	for (int i = 0; i < nrows * ncols; i++) {
		if (mask[i]) {
			index_matrix[i] = (int)imask.size();
			imask.push_back(i);
		}
	}
	int npix = (int)imask.size();

	// Helper: check if (r, c) is in mask
	auto inMask = [&](int r, int c) -> bool {
		if (r < 0 || r >= nrows || c < 0 || c >= ncols) return false;
		return mask[r * ncols + c] != 0;
	};

	typedef Triplet<double> T;
	vector<T> dx_trips, dy_trips;

	for (int k = 0; k < npix; k++) {
		int r = imask[k] / ncols;
		int c = imask[k] % ncols;
		int idx_c = index_matrix[r * ncols + c];

		// Dx: prefer forward (right neighbor), else backward (left)
		if (c + 1 < ncols && inMask(r, c + 1)) {
			int idx_r = index_matrix[r * ncols + c + 1];
			dx_trips.push_back(T(idx_c, idx_r, 1.0));
			dx_trips.push_back(T(idx_c, idx_c, -1.0));
		} else if (c - 1 >= 0 && inMask(r, c - 1)) {
			int idx_l = index_matrix[r * ncols + c - 1];
			dx_trips.push_back(T(idx_c, idx_c, 1.0));
			dx_trips.push_back(T(idx_c, idx_l, -1.0));
		}
		// else: no gradient (row stays zero)

		// Dy: prefer forward (bottom neighbor), else backward (top)
		if (r + 1 < nrows && inMask(r + 1, c)) {
			int idx_b = index_matrix[(r + 1) * ncols + c];
			dy_trips.push_back(T(idx_c, idx_b, 1.0));
			dy_trips.push_back(T(idx_c, idx_c, -1.0));
		} else if (r - 1 >= 0 && inMask(r - 1, c)) {
			int idx_t = index_matrix[(r - 1) * ncols + c];
			dy_trips.push_back(T(idx_c, idx_c, 1.0));
			dy_trips.push_back(T(idx_c, idx_t, -1.0));
		}
	}

	GradientOps ops;
	ops.npix = npix;
	ops.Dx.resize(npix, npix);
	ops.Dy.resize(npix, npix);
	ops.Dx.setFromTriplets(dx_trips.begin(), dx_trips.end());
	ops.Dy.setFromTriplets(dy_trips.begin(), dy_trips.end());
	return ops;
}

// ============================================================================
// t_fcn: compute the lighting field T and its gradient grad_T
// Corresponds to Eq. (3.14) in [1].
//
// z_tilde:   npix x 1 log-depth
// S:         nimgs x 3 light positions
// Dir:       nimgs x 3 light orientations
// mu:        nimgs x 1 anisotropy
// u_tilde, v_tilde: npix x 1 (pixel coords - principal point) / focal
// ============================================================================

struct TField {
	// T_field(pix, img, xyz) and grad_t(pix, img, xyz) stored as:
	// [npix x nimgs] per component (3 components)
	MatrixXd Tx, Ty, Tz;
	MatrixXd gTx, gTy, gTz;
};

static TField computeTField(const VectorXd &z_tilde,
                             const MatrixXd &S, const MatrixXd &Dir, const VectorXd &mu,
                             const VectorXd &u_tilde, const VectorXd &v_tilde) {
	int npix = (int)z_tilde.size();
	int nimgs = (int)S.rows();

	// Current 3D points: exp(z_tilde) * (u_tilde, v_tilde, 1)
	VectorXd exp_z = z_tilde.array().exp();
	VectorXd Xx = exp_z.array() * u_tilde.array();
	VectorXd Xy = exp_z.array() * v_tilde.array();
	VectorXd Xz = exp_z; // z component

	TField tf;
	tf.Tx.resize(npix, nimgs);
	tf.Ty.resize(npix, nimgs);
	tf.Tz.resize(npix, nimgs);
	tf.gTx.resize(npix, nimgs);
	tf.gTy.resize(npix, nimgs);
	tf.gTz.resize(npix, nimgs);

	for (int i = 0; i < nimgs; i++) {
		// Vector from surface point to light: S_i - XYZ
		VectorXd dx = VectorXd::Constant(npix, S(i, 0)) - Xx;
		VectorXd dy = VectorXd::Constant(npix, S(i, 1)) - Xy;
		VectorXd dz = VectorXd::Constant(npix, S(i, 2)) - Xz;

		// ||S_i - XYZ||
		VectorXd normS = (dx.array().square() + dy.array().square() + dz.array().square()).sqrt();

		// scal_prod = -(dx*Dir_x + dy*Dir_y + dz*Dir_z)  (dot product with negated direction)
		VectorXd scal_prod = -(dx.array() * Dir(i, 0) + dy.array() * Dir(i, 1) + dz.array() * Dir(i, 2));

		double mu_i = mu(i);

		// Attenuation: a = scal_prod^mu / ||S-X||^(3+mu)
		VectorXd a_field = scal_prod.array().pow(mu_i) / normS.array().pow(3.0 + mu_i);

		// da/dz (derivative of attenuation w.r.t. log-depth, chain rule applied)
		// XYZ dot Dir
		VectorXd xyz_dot_dir = Xx.array() * Dir(i, 0) + Xy.array() * Dir(i, 1) + Xz.array() * Dir(i, 2);
		// -(T dot XYZ)
		VectorXd neg_t_dot_xyz = -(dx.array() * Xx.array() + dy.array() * Xy.array() + dz.array() * Xz.array());

		VectorXd da_field = (mu_i * scal_prod.array().pow(mu_i - 1.0) * xyz_dot_dir.array()) /
		                     normS.array().pow(mu_i + 3.0)
		                   - (mu_i + 3.0) * scal_prod.array().pow(mu_i) * neg_t_dot_xyz.array() /
		                     normS.array().pow(mu_i + 5.0);

		// T_field(:,i,:) = (dx, dy, dz) .* a_field
		tf.Tx.col(i) = dx.array() * a_field.array();
		tf.Ty.col(i) = dy.array() * a_field.array();
		tf.Tz.col(i) = dz.array() * a_field.array();

		// grad_t(:,i,1) = -exp_z*u_tilde*(a + da) + da * S(i,1)
		tf.gTx.col(i) = -exp_z.array() * u_tilde.array() * (a_field.array() + da_field.array())
		                 + da_field.array() * S(i, 0);
		tf.gTy.col(i) = -exp_z.array() * v_tilde.array() * (a_field.array() + da_field.array())
		                 + da_field.array() * S(i, 1);
		tf.gTz.col(i) = -exp_z.array() * (a_field.array() + da_field.array())
		                 + da_field.array() * S(i, 2);
	}
	return tf;
}

// ============================================================================
// Simple inpaint-nans: fill NaN pixels with the average of their non-NaN
// neighbors, iteratively until all are filled.
// ============================================================================

static void inpaintNans(vector<double> &img, int nrows, int ncols) {
	bool changed = true;
	while (changed) {
		changed = false;
		vector<double> out = img;
		for (int r = 0; r < nrows; r++) {
			for (int c = 0; c < ncols; c++) {
				int idx = r * ncols + c;
				if (!isnan(img[idx])) continue;
				double sum = 0;
				int count = 0;
				if (r > 0 && !isnan(img[(r-1)*ncols+c])) { sum += img[(r-1)*ncols+c]; count++; }
				if (r < nrows-1 && !isnan(img[(r+1)*ncols+c])) { sum += img[(r+1)*ncols+c]; count++; }
				if (c > 0 && !isnan(img[r*ncols+c-1])) { sum += img[r*ncols+c-1]; count++; }
				if (c < ncols-1 && !isnan(img[r*ncols+c+1])) { sum += img[r*ncols+c+1]; count++; }
				if (count > 0) {
					out[idx] = sum / count;
					changed = true;
				}
			}
		}
		img = out;
	}
}

// ============================================================================
// Main solver: one scale
// ============================================================================

struct ScaleState {
	VectorXd z_tilde;       // npix log-depth
	MatrixXd rho_tilde;     // npix x nchannels pseudo-albedo
	MatrixXd Phi;           // nimgs x nchannels intensities
	vector<double> z_full;  // nrows x ncols depth
	vector<double> rho_full;// nrows x ncols x nchannels albedo
	vector<double> tab_nrj;
	bool cancelled = false;
};

static ScaleState solveAtScale(
	// Image data at current scale (already vectorized into masked pixels).
	// I_masked: npix x nimgs x nchannels (flattened as [pix][img][ch])
	const vector<double> &I_flat, // npix x nimgs x nchannels
	int npix, int nimgs, int nchannels,
	const vector<int> &imask,       // pixel indices within mask
	const vector<uint8_t> &mask,    // full nrows x ncols mask
	const vector<double> &used_flat,// npix x nimgs x nchannels usage weights
	int nrows, int ncols,
	double fx, double fy,
	const VectorXd &u_tilde_full,   // nrows x ncols u_tilde
	const VectorXd &v_tilde_full,   // nrows x ncols v_tilde
	const MatrixXd &S, const MatrixXd &Dir, const VectorXd &mu,
	MatrixXd Phi,
	const VectorXd &z0_log,         // npix initial log-depth
	const VectorXd &z_tilde_init,   // npix initial log-depth (from previous scale)
	const MatrixXd &rho_tilde_init, // npix x nchannels initial pseudo-albedo
	double lambda, double zeta,
	PhiFcn phi_fcn, WFcn w_fcn,
	bool self_shadows, bool semi_calibrated,
	int maxit, double tol, double tol_normals,
	double tol_pcg, int maxit_pcg,
	NearPSPrecond precond_type,
	function<bool(string, int)> progress)
{
	ScaleState state;
	state.Phi = Phi;

	// Extract per-pixel coordinate arrays
	VectorXd u_pix(npix), v_pix(npix);
	for (int k = 0; k < npix; k++) {
		u_pix(k) = u_tilde_full(imask[k]);
		v_pix(k) = v_tilde_full(imask[k]);
	}

	// Build gradient operators
	GradientOps gops = buildGradient(mask, nrows, ncols);
	const SparseMatrix<double> &Dx = gops.Dx;
	const SparseMatrix<double> &Dy = gops.Dy;

	// Initialize z_tilde and rho_tilde
	VectorXd z_tilde = z_tilde_init;
	MatrixXd rho_tilde = rho_tilde_init;

	// Helper to access I_flat: I(pix, img, ch) — stored as [pix * nimgs * nch + img * nch + ch]
	auto I = [&](int p, int i, int ch) -> double { return I_flat[(size_t)p * nimgs * nchannels + i * nchannels + ch]; };
	auto W = [&](int p, int i, int ch) -> double { return used_flat[(size_t)p * nimgs * nchannels + i * nchannels + ch]; };

	// Self-shadow functions
	auto psi_fcn = [&](double x) -> double { return self_shadows ? max(x, 0.0) : x; };
	auto chi_fcn = [&](double x) -> double { return self_shadows ? (x >= 0.0 ? 1.0 : 0.0) : 1.0; };

	// Compute shading: for each (pix, img), shading = Tx*zx + Ty*zy - Tz term
	// shading(p, i) = (fx*Tx(p,i) - u_pix(p)*Tz(p,i)) * zx(p) + (fy*Ty(p,i) - v_pix(p)*Tz(p,i)) * zy(p) - Tz(p,i)
	// where zx = Dx * z_tilde, zy = Dy * z_tilde

	auto computeShading = [&](const VectorXd &zt, const TField &tf) -> MatrixXd {
		VectorXd zx = Dx * zt;
		VectorXd zy = Dy * zt;
		MatrixXd psi(npix, nimgs);
		for (int i = 0; i < nimgs; i++) {
			for (int p = 0; p < npix; p++) {
				psi(p, i) = (fx * tf.Tx(p, i) - u_pix(p) * tf.Tz(p, i)) * zx(p)
				           + (fy * tf.Ty(p, i) - v_pix(p) * tf.Tz(p, i)) * zy(p)
				           - tf.Tz(p, i);
			}
		}
		return psi;
	};

	// Compute residual r(p, i, ch) = W * (rho * Phi * psi_fcn(shading) - I)
	// Compute energy = sum phi(r)
	auto computeEnergy = [&](const MatrixXd &psi_mat, const MatrixXd &rho_t,
	                          const MatrixXd &cur_Phi) -> double {
		double energy = 0.0;
		for (int ch = 0; ch < nchannels; ch++) {
			for (int i = 0; i < nimgs; i++) {
				double phi_ch_i = cur_Phi(i, ch);
				for (int p = 0; p < npix; p++) {
					double w_mask = W(p, i, ch);
					if (w_mask <= 0) continue;
					double r = w_mask * (rho_t(p, ch) * phi_ch_i * psi_fcn(psi_mat(p, i)) - I(p, i, ch));
					energy += phi_fcn(r, lambda);
				}
			}
		}
		return energy / (double(npix) * nimgs * nchannels);
	};

	// Compute normals from z_tilde
	auto computeNormals = [&](const VectorXd &zt) -> vector<Vector3d> {
		VectorXd zx = Dx * zt;
		VectorXd zy = Dy * zt;
		vector<Vector3d> N(npix);
		for (int p = 0; p < npix; p++) {
			double nx = fx * zx(p);
			double ny = fy * zy(p);
			double nz = -u_pix(p) * zx(p) - v_pix(p) * zy(p) - 1.0;
			double len = sqrt(nx * nx + ny * ny + nz * nz);
			if (len > 0) N[p] = Vector3d(nx / len, ny / len, nz / len);
			else N[p] = Vector3d(0, 0, -1);
		}
		return N;
	};

	// Initial T field and shading
	TField tf = computeTField(z_tilde, S, Dir, mu,
	                          u_pix.array() / fx, v_pix.array() / fy);
	MatrixXd psi = computeShading(z_tilde, tf);
	double energy = computeEnergy(psi, rho_tilde, state.Phi);
	energy += zeta * (z_tilde - z0_log).squaredNorm() / npix;

	state.tab_nrj.push_back(energy);

	vector<Vector3d> N_old = computeNormals(z_tilde);

	cout << "  it. 0 - energy: " << energy << endl;

	// Replicated gradient operators (nimgs copies stacked vertically)
	SparseMatrix<double> Dx_rep(npix * nimgs, npix);
	SparseMatrix<double> Dy_rep(npix * nimgs, npix);
	{
		typedef Triplet<double> T;
		vector<T> dx_tr, dy_tr;
		// Reserve: Dx has nnz per block, nimgs blocks
		dx_tr.reserve(Dx.nonZeros() * nimgs);
		dy_tr.reserve(Dy.nonZeros() * nimgs);
		for (int i = 0; i < nimgs; i++) {
			int row_off = i * npix;
			for (int k = 0; k < Dx.outerSize(); k++) {
				for (SparseMatrix<double>::InnerIterator it(Dx, k); it; ++it)
					dx_tr.push_back(T(row_off + (int)it.row(), (int)it.col(), it.value()));
			}
			for (int k = 0; k < Dy.outerSize(); k++) {
				for (SparseMatrix<double>::InnerIterator it(Dy, k); it; ++it)
					dy_tr.push_back(T(row_off + (int)it.row(), (int)it.col(), it.value()));
			}
		}
		Dx_rep.setFromTriplets(dx_tr.begin(), dx_tr.end());
		Dy_rep.setFromTriplets(dy_tr.begin(), dy_tr.end());
	}

	// Weight matrix for all (pix, img, ch)
	MatrixXd w_mat(npix, nimgs); // reused per channel

	// Main iteration loop
	for (int it = 1; it <= maxit; it++) {

		// Compute chi for current shading
		MatrixXd chi_mat(npix, nimgs);
		MatrixXd phi_chi(npix, nimgs);
		for (int i = 0; i < nimgs; i++) {
			for (int p = 0; p < npix; p++) {
				chi_mat(p, i) = chi_fcn(psi(p, i));
				phi_chi(p, i) = psi_fcn(psi(p, i)) * chi_mat(p, i);
			}
		}

		// ---- Pseudo-albedo update (per channel) ----
		for (int ch = 0; ch < nchannels; ch++) {
			// Compute weights
			for (int i = 0; i < nimgs; i++) {
				double phi_i = state.Phi(i, ch);
				for (int p = 0; p < npix; p++) {
					double w_mask = W(p, i, ch);
					double r = w_mask * (rho_tilde(p, ch) * phi_i * psi_fcn(psi(p, i)) - I(p, i, ch));
					w_mat(p, i) = w_mask * w_fcn(r, lambda);
				}
			}
			// phi_chi_ch(p, i) = psi(p,i)*chi(p,i) * Phi(i,ch)
			MatrixXd phi_chi_ch(npix, nimgs);
			for (int i = 0; i < nimgs; i++) {
				double phi_i = state.Phi(i, ch);
				for (int p = 0; p < npix; p++) {
					phi_chi_ch(p, i) = phi_chi(p, i) * phi_i;
				}
			}
			for (int p = 0; p < npix; p++) {
				double denom = 0, numer = 0;
				for (int i = 0; i < nimgs; i++) {
					denom += w_mat(p, i) * phi_chi_ch(p, i) * phi_chi_ch(p, i);
					numer += w_mat(p, i) * I(p, i, ch) * phi_chi_ch(p, i);
				}
				if (denom > 0) rho_tilde(p, ch) = numer / denom;
			}
		}

		// ---- Log-depth update ----
		// Recompute weights with updated albedo
		// Build the system M * dz = rhs

		// We need rho_rep(p, img*nchannels block) and D weights
		// D(p, j) for j in [0, nimgs*nchannels) = chi * rho_rep^2 * w
		VectorXd D_vec(npix * nimgs * nchannels);
		VectorXd rhs_vec(npix * nimgs * nchannels);

		for (int ch = 0; ch < nchannels; ch++) {
			for (int i = 0; i < nimgs; i++) {
				double phi_i = state.Phi(i, ch);
				for (int p = 0; p < npix; p++) {
					double w_mask = W(p, i, ch);
					double rp = rho_tilde(p, ch) * phi_i;
					double r = w_mask * (rp * psi_fcn(psi(p, i)) - I(p, i, ch));
					double w_val = w_mask * w_fcn(r, lambda);

					int idx = ch * npix * nimgs + i * npix + p;
					D_vec(idx) = chi_mat(p, i) * rp * rp * w_val;
					rhs_vec(idx) = chi_mat(p, i) * rp * (rp * (-psi(p, i)) + I(p, i, ch)) * w_val;
				}
			}
		}

		// Build the A matrix (npix*nimgs x npix) from the linearization
		// A = diag(fx*Tx - px*Tz) * Dx_rep + diag(fy*Ty - py*Tz) * Dy_rep
		// plus the gradient correction term
		VectorXd coeffX(npix * nimgs), coeffY(npix * nimgs);
		VectorXd gcoeffX(npix * nimgs), gcoeffY(npix * nimgs);
		VectorXd tz_flat(npix * nimgs), gtz_flat(npix * nimgs);

		for (int i = 0; i < nimgs; i++) {
			for (int p = 0; p < npix; p++) {
				int idx = i * npix + p;
				coeffX(idx) = fx * tf.Tx(p, i) - u_pix(p) * tf.Tz(p, i);
				coeffY(idx) = fy * tf.Ty(p, i) - v_pix(p) * tf.Tz(p, i);
				gcoeffX(idx) = fx * tf.gTx(p, i) - u_pix(p) * tf.gTz(p, i);
				gcoeffY(idx) = fy * tf.gTy(p, i) - v_pix(p) * tf.gTz(p, i);
				tz_flat(idx) = tf.Tz(p, i);
				gtz_flat(idx) = tf.gTz(p, i);
			}
		}

		SparseMatrix<double> A = coeffX.asDiagonal() * Dx_rep + coeffY.asDiagonal() * Dy_rep;

		// gradient correction: grad_shading * z_tilde projected onto each pixel
		VectorXd grad_shading_z = (gcoeffX.asDiagonal() * Dx_rep + gcoeffY.asDiagonal() * Dy_rep) * z_tilde - gtz_flat;

		// Build the selection matrix: col(p) repeated to each (i*npix + p)
		// This is a sparse npix*nimgs x npix matrix mapping pixel p to all its image copies
		// Instead of building it explicitly, add to A: A(i*npix+p, p) += grad_shading_z(i*npix+p)
		{
			typedef Triplet<double> T;
			vector<T> corr_trips;
			corr_trips.reserve(npix * nimgs);
			for (int i = 0; i < nimgs; i++) {
				for (int p = 0; p < npix; p++) {
					corr_trips.push_back(T(i * npix + p, p, grad_shading_z(i * npix + p)));
				}
			}
			SparseMatrix<double> corr(npix * nimgs, npix);
			corr.setFromTriplets(corr_trips.begin(), corr_trips.end());
			A = A + corr;
		}

		// Replicate A for all channels: A_full = [A; A; ... A] (nchannels copies)
		SparseMatrix<double> A_full;
		if (nchannels == 1) {
			A_full = A;
		} else {
			typedef Triplet<double> T;
			vector<T> af_trips;
			af_trips.reserve(A.nonZeros() * nchannels);
			for (int ch = 0; ch < nchannels; ch++) {
				int row_off = ch * npix * nimgs;
				for (int k = 0; k < A.outerSize(); k++) {
					for (SparseMatrix<double>::InnerIterator it(A, k); it; ++it)
						af_trips.push_back(T(row_off + (int)it.row(), (int)it.col(), it.value()));
				}
			}
			A_full.resize(nchannels * npix * nimgs, npix);
			A_full.setFromTriplets(af_trips.begin(), af_trips.end());
		}

		// M = At * diag(D) * A / (nimgs*npix*nchannels) + zeta * I / npix
		SparseMatrix<double> AtDA = A_full.transpose() * D_vec.asDiagonal() * A_full;
		AtDA /= double(nimgs * npix * nchannels);
		// Add zeta regularization
		{
			typedef Triplet<double> T;
			vector<T> reg_trips;
			for (int p = 0; p < npix; p++)
				reg_trips.push_back(T(p, p, zeta / npix));
			SparseMatrix<double> reg(npix, npix);
			reg.setFromTriplets(reg_trips.begin(), reg_trips.end());
			AtDA = AtDA + reg;
		}

		// rhs = At * rhs_vec / (nimgs*npix*nchannels) + zeta*(z0 - z_tilde)/npix
		VectorXd rhs = A_full.transpose() * rhs_vec / double(nimgs * npix * nchannels);
		rhs += zeta * (z0_log - z_tilde) / npix;

		// Solve with PCG
		ConjugateGradient<SparseMatrix<double>, Lower|Upper> cg;
		cg.setTolerance(tol_pcg);
		cg.setMaxIterations(maxit_pcg);
		cg.compute(AtDA);
		VectorXd dz = cg.solve(rhs);
		z_tilde += dz;

		// ---- Intensities update (semi-calibrated) ----
		if (semi_calibrated) {
			for (int ch = 0; ch < nchannels; ch++) {
				for (int i = 0; i < nimgs; i++) {
					double numer = 0, denom = 0;
					for (int p = 0; p < npix; p++) {
						double w_mask = W(p, i, ch);
						double rp = rho_tilde(p, ch);
						double r = w_mask * (rp * state.Phi(i, ch) * psi_fcn(psi(p, i)) - I(p, i, ch));
						double w_val = w_mask * max(0.0, w_fcn(r, lambda));
						double rho_psi_chi_val = psi_fcn(psi(p, i)) * chi_mat(p, i) * rp;
						numer += w_val * I(p, i, ch) * rho_psi_chi_val;
						denom += w_val * rho_psi_chi_val * rho_psi_chi_val;
					}
					if (denom > 0) state.Phi(i, ch) = numer / denom;
				}
			}
		}

		// Recompute T field, shading, normals
		tf = computeTField(z_tilde, S, Dir, mu,
		                   u_pix.array() / fx, v_pix.array() / fy);
		psi = computeShading(z_tilde, tf);
		vector<Vector3d> N_new = computeNormals(z_tilde);

		// Energy
		double energy_new = computeEnergy(psi, rho_tilde, state.Phi);
		energy_new += zeta * (z_tilde - z0_log).squaredNorm() / npix;
		double relative_diff = fabs(energy_new - energy) / max(fabs(energy_new), 1e-30);

		// Normal convergence
		double median_ang = 0;
		{
			vector<double> angles(npix);
			for (int p = 0; p < npix; p++) {
				double dot = N_old[p].dot(N_new[p]);
				dot = min(1.0, max(-1.0, dot));
				angles[p] = acos(dot) * 180.0 / M_PI;
			}
			nth_element(angles.begin(), angles.begin() + npix / 2, angles.end());
			median_ang = angles[npix / 2];
		}

		bool diverged = energy_new > energy;

		cout << "  it. " << it
		     << " - energy: " << energy_new
		     << " - rel diff: " << relative_diff
		     << " - normal median: " << median_ang << endl;

		energy = energy_new;
		state.tab_nrj.push_back(energy);
		N_old = N_new;

		if (progress) {
			int pct = min(99, int(100.0 * it / maxit));
			if (!progress("Near PS iteration " + to_string(it), pct)) {
				state.cancelled = true;
				break;
			}
		}

		if (it > 1 && relative_diff < tol) {
			cout << "  CONVERGENCE: energy evolution is low enough" << endl;
			break;
		}
		if (it > 1 && median_ang < tol_normals) {
			cout << "  CONVERGENCE: shape evolution is low enough" << endl;
			break;
		}
		if (it > 1 && diverged) {
			cout << "  STOPPED: divergent behavior" << endl;
			break;
		}
		if (it == maxit) {
			cout << "  STOPPED: max iterations reached" << endl;
		}
	}

	// Store results as full images
	state.z_tilde = z_tilde;
	state.rho_tilde = rho_tilde;

	// Expand to full grid
	state.z_full.assign(nrows * ncols, NAN);
	for (int k = 0; k < npix; k++)
		state.z_full[imask[k]] = exp(z_tilde(k));

	state.rho_full.assign(nrows * ncols * nchannels, 0.0);
	{
		VectorXd zx = Dx * z_tilde;
		VectorXd zy = Dy * z_tilde;
		for (int k = 0; k < npix; k++) {
			double nx_v = fx * zx(k);
			double ny_v = fy * zy(k);
			double nz_v = -u_pix(k) * zx(k) - v_pix(k) * zy(k) - 1.0;
			double dz_v = sqrt(nx_v * nx_v + ny_v * ny_v + nz_v * nz_v);
			for (int ch = 0; ch < nchannels; ch++) {
				double rho_val = rho_tilde(k, ch) * dz_v;
				// Clamp
				rho_val = max(0.0, rho_val);
				state.rho_full[imask[k] * nchannels + ch] = rho_val;
			}
		}
	}

	return state;
}

// ============================================================================
// Public API
// ============================================================================

NearPSResult near_ps(const NearPSData &data,
                     const NearPSCalib &calib,
                     const NearPSParams &params,
                     function<bool(string, int)> progress) {
	NearPSResult result;

	int nrows = data.nrows;
	int ncols = data.ncols;
	int nimgs = data.nimgs;
	int nchannels = data.nchannels;

	if (nrows <= 0 || ncols <= 0 || nimgs <= 0) {
		cerr << "near_ps ERROR: invalid dimensions" << endl;
		return result;
	}

	// ---- Calibration ----
	MatrixXd S = calib.S;
	if (S.rows() != nimgs || S.cols() != 3) {
		cerr << "near_ps ERROR: S should be nimgs x 3" << endl;
		return result;
	}

	Matrix3d K = calib.K;
	if (K(0, 2) == 0) K.transposeInPlace(); // K(1,3) in MATLAB 1-based = K(0,2) in C++ 0-based
	double fx = K(0, 0);
	double fy = K(1, 1);
	double x0 = K(0, 2);
	double y0 = K(1, 2);

	MatrixXd Phi = calib.Phi;
	if (Phi.rows() == 0) {
		Phi = MatrixXd::Ones(nimgs, nchannels);
	}
	if (Phi.cols() == 1 && nchannels > 1) {
		Phi = Phi.col(0).replicate(1, nchannels);
	}

	VectorXd mu = calib.mu;
	if (mu.size() == 0) mu = VectorXd::Zero(nimgs);

	MatrixXd Dir = calib.Dir;
	if (Dir.rows() == 0) {
		Dir = MatrixXd::Zero(nimgs, 3);
		Dir.col(2).setOnes();
	}

	// ---- Mask ----
	vector<uint8_t> mask(nrows * ncols, 1);
	if (!data.mask.empty()) {
		for (int i = 0; i < nrows * ncols; i++)
			mask[i] = data.mask[i] ? 1 : 0;
	}

	// ---- Used ----
	int total_data_size = nrows * ncols * nimgs * nchannels;
	vector<uint8_t> used(total_data_size, 1);
	if (!data.used.empty()) {
		for (int i = 0; i < total_data_size; i++)
			used[i] = data.used[i];
	}

	// ---- Images ----
	// Normalize to [0, 1]
	double max_I = *max_element(data.I.begin(), data.I.end());
	if (max_I <= 0) max_I = 1.0;
	vector<double> I_norm(data.I.size());
	for (size_t i = 0; i < data.I.size(); i++)
		I_norm[i] = data.I[i] / max_I;
	Phi /= max_I;

	// ---- z0 ----
	vector<double> z0(nrows * ncols, params.default_z0);
	if (!params.z0.empty()) {
		z0 = params.z0;
	}

	// ---- Downsampling ratio ----
	int ratio = max(1, params.ratio);
	if (ratio > 1) {
		int new_nrows = nrows / ratio;
		int new_ncols = ncols / ratio;

		// Resize mask
		vector<uint8_t> new_mask(new_nrows * new_ncols);
		resizeMask(mask.data(), ncols, nrows, new_mask.data(), new_ncols, new_nrows);
		mask = new_mask;

		// Resize z0
		vector<double> new_z0(new_nrows * new_ncols);
		resizeImage(z0.data(), ncols, nrows, new_z0.data(), new_ncols, new_nrows);
		z0 = new_z0;

		// Resize images
		vector<double> new_I(new_nrows * new_ncols * nimgs * nchannels);
		vector<uint8_t> new_used(new_nrows * new_ncols * nimgs * nchannels, 1);
		for (int img = 0; img < nimgs; img++) {
			for (int ch = 0; ch < nchannels; ch++) {
				// Extract plane
				vector<double> plane(nrows * ncols);
				vector<double> uplane(nrows * ncols);
				for (int r = 0; r < nrows; r++) {
					for (int c = 0; c < ncols; c++) {
						int old_idx;
						if (nchannels == 1)
							old_idx = img * nrows * ncols + r * ncols + c;
						else
							old_idx = img * nrows * ncols * nchannels + ch * nrows * ncols + r * ncols + c;
						plane[r * ncols + c] = I_norm[old_idx];
						uplane[r * ncols + c] = used[old_idx];
					}
				}
				vector<double> rplane(new_nrows * new_ncols);
				resizeImage(plane.data(), ncols, nrows, rplane.data(), new_ncols, new_nrows);
				vector<double> ruplane(new_nrows * new_ncols);
				resizeImage(uplane.data(), ncols, nrows, uplane.data(), new_ncols, new_nrows);
				for (int r = 0; r < new_nrows; r++) {
					for (int c = 0; c < new_ncols; c++) {
						int new_idx;
						if (nchannels == 1)
							new_idx = img * new_nrows * new_ncols + r * new_ncols + c;
						else
							new_idx = img * new_nrows * new_ncols * nchannels + ch * new_nrows * new_ncols + r * new_ncols + c;
						new_I[new_idx] = rplane[r * new_ncols + c];
						new_used[new_idx] = ruplane[r * new_ncols + c] > 0.5 ? 1 : 0;
					}
				}
			}
		}
		I_norm = new_I;
		used = new_used;

		fx /= ratio;
		fy /= ratio;
		x0 /= ratio;
		y0 /= ratio;
		nrows = new_nrows;
		ncols = new_ncols;
	}

	// ---- Multi-scale setup ----
	int num_scales = max(1, params.scales);
	vector<int> scale_factors;
	for (int s = num_scales - 1; s >= 0; s--)
		scale_factors.push_back(1 << s);

	// Store reference data
	double fx_ref = fx, fy_ref = fy, x0_ref = x0, y0_ref = y0;
	vector<double> I_ref = I_norm;
	vector<uint8_t> mask_ref = mask;
	vector<uint8_t> used_ref = used;
	vector<double> z0_ref = z0;
	int nrows_ref = nrows, ncols_ref = ncols;
	double lambda_ref = params.lambda;

	// Initialize depth/albedo for propagation between scales
	vector<double> z_final = z0;
	vector<double> rho_final(nrows * ncols * nchannels, 0.0);

	// Estimator function pointers
	PhiFcn phi_f; WFcn w_f;
	selectEstimator(params.estimator, phi_f, w_f);

	// ---- Scale loop ----
	for (int si = 0; si < (int)scale_factors.size(); si++) {
		int scale = scale_factors[si];

		// Scale data
		double cur_fx = fx_ref / scale;
		double cur_fy = fy_ref / scale;
		double cur_x0 = x0_ref / scale;
		double cur_y0 = y0_ref / scale;

		int cur_nrows, cur_ncols;
		vector<uint8_t> cur_mask;
		vector<double> cur_z0;
		vector<double> cur_I;
		vector<uint8_t> cur_used;

		if (scale > 1) {
			cur_nrows = nrows_ref / scale;
			cur_ncols = ncols_ref / scale;

			cur_mask.resize(cur_nrows * cur_ncols);
			resizeMask(mask_ref.data(), ncols_ref, nrows_ref, cur_mask.data(), cur_ncols, cur_nrows);

			cur_z0.resize(cur_nrows * cur_ncols);
			resizeImage(z0_ref.data(), ncols_ref, nrows_ref, cur_z0.data(), cur_ncols, cur_nrows);

			// Resize all image planes
			int plane_sz = nrows_ref * ncols_ref;
			int new_plane_sz = cur_nrows * cur_ncols;
			int total_planes = nimgs * nchannels;
			cur_I.resize(new_plane_sz * total_planes);
			cur_used.resize(new_plane_sz * total_planes, 1);
			for (int pl = 0; pl < total_planes; pl++) {
				vector<double> plane(plane_sz);
				vector<double> uplane(plane_sz);
				for (int k = 0; k < plane_sz; k++) {
					cur_I[k] = I_ref[pl * plane_sz + k]; // temp
					uplane[k] = used_ref[pl * plane_sz + k];
				}
				// Actually copy and resize
				for (int k = 0; k < plane_sz; k++)
					plane[k] = I_ref[pl * plane_sz + k];

				vector<double> rp(new_plane_sz), rup(new_plane_sz);
				resizeImage(plane.data(), ncols_ref, nrows_ref, rp.data(), cur_ncols, cur_nrows);
				resizeImage(uplane.data(), ncols_ref, nrows_ref, rup.data(), cur_ncols, cur_nrows);
				for (int k = 0; k < new_plane_sz; k++) {
					cur_I[pl * new_plane_sz + k] = rp[k];
					cur_used[pl * new_plane_sz + k] = rup[k] > 0.5 ? 1 : 0;
				}
			}
		} else {
			cur_nrows = nrows_ref;
			cur_ncols = ncols_ref;
			cur_mask = mask_ref;
			cur_z0 = z0_ref;
			cur_I = I_ref;
			cur_used.resize(used_ref.size());
			for (size_t k = 0; k < used_ref.size(); k++)
				cur_used[k] = used_ref[k];
		}

		// Compute pixel coordinates
		VectorXd u_tilde(cur_nrows * cur_ncols), v_tilde(cur_nrows * cur_ncols);
		for (int r = 0; r < cur_nrows; r++) {
			for (int c = 0; c < cur_ncols; c++) {
				u_tilde(r * cur_ncols + c) = (c + 1) - cur_x0;  // MATLAB 1-indexed
				v_tilde(r * cur_ncols + c) = (r + 1) - cur_y0;
			}
		}

		// Build mask indices
		vector<int> imask;
		for (int i = 0; i < cur_nrows * cur_ncols; i++)
			if (cur_mask[i]) imask.push_back(i);
		int npix = (int)imask.size();

		// Vectorize images: reorganize from spatial layout to per-masked-pixel
		// Output: I_masked[p * nimgs * nchannels + img * nchannels + ch]
		// Input layout: I_ref is [img * nrows*ncols*nchannels + ch*nrows*ncols + r*ncols + c] for color
		//              or [img * nrows*ncols + r*ncols + c] for grayscale
		vector<double> I_masked(npix * nimgs * nchannels);
		vector<double> used_masked(npix * nimgs * nchannels, 1.0);
		for (int p = 0; p < npix; p++) {
			int pixel = imask[p];
			for (int img = 0; img < nimgs; img++) {
				for (int ch = 0; ch < nchannels; ch++) {
					int src_idx;
					if (nchannels == 1)
						src_idx = img * cur_nrows * cur_ncols + pixel;
					else
						src_idx = img * cur_nrows * cur_ncols * nchannels + ch * cur_nrows * cur_ncols + pixel;
					I_masked[p * nimgs * nchannels + img * nchannels + ch] = cur_I[src_idx];

					int used_idx;
					if (nchannels == 1)
						used_idx = img * cur_nrows * cur_ncols + pixel;
					else
						used_idx = img * cur_nrows * cur_ncols * nchannels + ch * cur_nrows * cur_ncols + pixel;
					used_masked[p * nimgs * nchannels + img * nchannels + ch] = cur_used[used_idx];
				}
			}
		}

		// Scale lambda based on median absolute deviation
		double cur_lambda = lambda_ref;
		if (params.estimator != NearPSEstimator::Lp && params.estimator != NearPSEstimator::LS) {
			// median absolute deviation of all image values
			vector<double> abs_dev;
			double med_val;
			{
				vector<double> vals(I_masked.begin(), I_masked.end());
				nth_element(vals.begin(), vals.begin() + vals.size() / 2, vals.end());
				med_val = vals[vals.size() / 2];
			}
			{
				vector<double> devs(I_masked.size());
				for (size_t k = 0; k < I_masked.size(); k++)
					devs[k] = fabs(I_masked[k] - med_val);
				nth_element(devs.begin(), devs.begin() + devs.size() / 2, devs.end());
				cur_lambda = lambda_ref * devs[devs.size() / 2];
			}
		}

		// Resize z_final to current scale for initialization
		vector<double> z_cur(cur_nrows * cur_ncols);
		if (scale > 1 || si > 0) {
			resizeImage(z_final.data(), ncols_ref, nrows_ref, z_cur.data(), cur_ncols, cur_nrows);
		} else {
			z_cur = z_final;
		}

		// Initialize log-depth and pseudo-albedo for masked pixels
		VectorXd z0_log(npix), z_tilde_init(npix);
		for (int k = 0; k < npix; k++) {
			double zv = max(1e-6, cur_z0[imask[k]]);
			z0_log(k) = log(zv);
			double zc = max(1e-6, z_cur[imask[k]]);
			z_tilde_init(k) = log(zc);
		}

		// Initialize rho_tilde (pseudo-albedo): rho / dz_norm
		// For first scale, init to zero (will be solved in first iteration)
		MatrixXd rho_tilde_init = MatrixXd::Zero(npix, nchannels);

		// If we have rho from previous scale, resize and use
		if (si > 0) {
			vector<double> rho_cur(cur_nrows * cur_ncols * nchannels);
			for (int ch = 0; ch < nchannels; ch++) {
				vector<double> rho_plane(nrows_ref * ncols_ref);
				for (int k = 0; k < nrows_ref * ncols_ref; k++)
					rho_plane[k] = rho_final[k * nchannels + ch];
				vector<double> rho_resized(cur_nrows * cur_ncols);
				resizeImage(rho_plane.data(), ncols_ref, nrows_ref, rho_resized.data(), cur_ncols, cur_nrows);
				for (int k = 0; k < cur_nrows * cur_ncols; k++)
					rho_cur[k * nchannels + ch] = rho_resized[k];
			}
			// Convert to pseudo-albedo: rho_tilde = rho / dz
			// For simplicity, just use rho as-is (the solver will correct)
			for (int k = 0; k < npix; k++) {
				for (int ch = 0; ch < nchannels; ch++) {
					rho_tilde_init(k, ch) = rho_cur[imask[k] * nchannels + ch];
				}
			}
		}

		cout << "=== Scale " << scale << ": " << cur_nrows << "x" << cur_ncols
		     << " (" << npix << " pixels, " << nimgs << " images, "
		     << nchannels << " channels, lambda=" << cur_lambda << ")" << endl;

		// ---- Solve ----
		ScaleState ss = solveAtScale(
			I_masked, npix, nimgs, nchannels, imask, cur_mask, used_masked,
			cur_nrows, cur_ncols,
			cur_fx, cur_fy, u_tilde, v_tilde,
			S, Dir, mu, Phi,
			z0_log, z_tilde_init, rho_tilde_init,
			cur_lambda, params.zeta, phi_f, w_f,
			params.self_shadows, params.semi_calibrated,
			params.maxit, params.tol, params.tol_normals,
			params.tol_pcg, params.maxit_pcg,
			params.precond,
			progress);

		Phi = ss.Phi;

		if (ss.cancelled) break;

		// Upscale for next level
		if (scale > 1) {
			// Fill NaN with inpaint
			inpaintNans(ss.z_full, cur_nrows, cur_ncols);
			z_final.resize(nrows_ref * ncols_ref);
			resizeImage(ss.z_full.data(), cur_ncols, cur_nrows, z_final.data(), ncols_ref, nrows_ref);
			rho_final.resize(nrows_ref * ncols_ref * nchannels);
			for (int ch = 0; ch < nchannels; ch++) {
				vector<double> rp(cur_nrows * cur_ncols);
				for (int k = 0; k < cur_nrows * cur_ncols; k++)
					rp[k] = ss.rho_full[k * nchannels + ch];
				vector<double> rr(nrows_ref * ncols_ref);
				resizeImage(rp.data(), cur_ncols, cur_nrows, rr.data(), ncols_ref, nrows_ref);
				for (int k = 0; k < nrows_ref * ncols_ref; k++)
					rho_final[k * nchannels + ch] = rr[k];
			}
		} else {
			z_final = ss.z_full;
			rho_final = ss.rho_full;
		}

		// Keep energy history from the last scale
		result.tab_nrj = ss.tab_nrj;
	}

	// ---- Build output ----
	result.nrows = nrows_ref;
	result.ncols = ncols_ref;
	result.nchannels = nchannels;

	// Depth
	result.Z.resize(nrows_ref * ncols_ref);
	for (int i = 0; i < nrows_ref * ncols_ref; i++)
		result.Z[i] = isnan(z_final[i]) ? 0.0 : z_final[i];

	// Normals: recompute from final depth via finite differences
	result.N.resize(nrows_ref * ncols_ref, Vector3d(0, 0, -1));
	for (int r = 0; r < nrows_ref; r++) {
		for (int c = 0; c < ncols_ref; c++) {
			if (!mask_ref[r * ncols_ref + c]) continue;
			int idx = r * ncols_ref + c;
			double z = z_final[idx];
			if (isnan(z) || z <= 0) continue;

			// Finite differences in depth
			double dzdx = 0, dzdy = 0;
			if (c + 1 < ncols_ref && mask_ref[idx + 1] && !isnan(z_final[idx + 1]))
				dzdx = z_final[idx + 1] - z;
			else if (c > 0 && mask_ref[idx - 1] && !isnan(z_final[idx - 1]))
				dzdx = z - z_final[idx - 1];

			if (r + 1 < nrows_ref && mask_ref[idx + ncols_ref] && !isnan(z_final[idx + ncols_ref]))
				dzdy = z_final[idx + ncols_ref] - z;
			else if (r > 0 && mask_ref[idx - ncols_ref] && !isnan(z_final[idx - ncols_ref]))
				dzdy = z - z_final[idx - ncols_ref];

			Vector3d n(-dzdx, -dzdy, 1.0);
			n.normalize();
			result.N[idx] = n;
		}
	}

	// Albedo
	result.rho = rho_final;

	// Phi: back-scale
	result.Phi = Phi * max_I;

	// Mask
	result.mask = mask_ref;

	return result;
}
