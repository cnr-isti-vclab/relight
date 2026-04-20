#ifndef NORMALSWORKER_H
#define NORMALSWORKER_H

#include "../imageset.h"
#include "normals_parameters.h"

#include <Eigen/Core>
#include <utility>
#include <vector>

#include <QMutex>

// KNN graph on light directions: knn[i] is a list of (proximity_weight, index) pairs
// for the k nearest neighbours of light i, sorted closest-first.
using LightKNN = std::vector<std::vector<std::pair<float, int>>>;

class NormalsWorker
{
public:
	NormalsWorker(NormalSolver _solver, int _row, const PixelArray& toProcess, Eigen::Vector3f* normals, ImageSet &imageset,
	              float highThreshold = 250.0f, float lowThreshold = 5.0f):
		solver(_solver), row(_row), m_Row(toProcess), m_Normals(normals), m_Imageset(imageset),
		robust_threshold_high(highThreshold), robust_threshold_low(lowThreshold) {
		m_Row.resize(toProcess.npixels(), toProcess.nlights);
		for(size_t i = 0; i < m_Row.size(); i++)
			m_Row[i] = toProcess[i];
	}

	void run();

	// Optional output: per-pixel per-light solver weights.
	// Layout: weights_out[pixelIndex * nlights + lightIndex].
	// Must point to a buffer of size npixels * nlights floats (caller allocates).
	// If nullptr the weights are computed but not stored.
	// Semantics per solver:
	//   L2          – 1.0 for every light
	//   Robust      – final Huber weight for kept lights; 0.0 for discarded lights
	//   Lambertian  – 1.0 - shadow_probability for each light
	void setWeightsOutput(float* ptr) { m_WeightsOut = ptr; }

	// Build a k-nearest-neighbour graph on pre-normalised light directions using
	// angular proximity (dot product). Each entry knn[i][m] = {weight, j} where
	// weight = dot(light_i, light_j) ∈ (0, 1]. Call this once per row/batch and
	// reuse the result across per-pixel calls to computeShadowMask.
	static LightKNN buildLightKNN(const std::vector<Eigen::Vector3f> &normalizedLights, int k = 10);

	// Compute a unit normal for one pixel via an L2 fit on the middle 60% of
	// light intensities (bottom and top 20% trimmed to reduce shadow/highlight
	// influence).  Requires at least 5 samples after trimming; if fewer are
	// available, falls back to (0, 0, 1).
	//
	// pixelIndex        : index into m_Row
	// normalizedLights  : pre-normalised light directions
	Eigen::Vector3f solveL2Trimmed(int pixelIndex,
	                               const std::vector<Eigen::Vector3f> &normalizedLights) const;

	// Returns a per-light shadow/highlight probability vector of length nlights
	// for a single pixel, where entry m ∈ [0, 1]:
	//   0 = consistent with Lambertian model
	//   1 = very likely shadow or highlight
	//
	// Algorithm:
	//  1. Compute expected Lambertian cosines λ[m] = max(0, l_m · normal).
	//  2. Compute normalised albedo estimates nval[m] = val[m] / λ[m].
	//  3. Iteratively remove the light with the highest mean KNN edge weight
	//     (log-ratio of neighbouring nval entries) until all remaining edges
	//     are below a threshold, yielding the largest consistent subset.
	//  4. Removal order is converted to a continuous probability.
	//
	// pixelIndex       : index into m_Row for this pixel
	// normal           : surface normal for this pixel
	// normalizedLights : pre-normalised light directions (same as passed to buildLightKNN)
	// knn              : pre-built KNN graph from buildLightKNN
	Eigen::VectorXf computeShadowMask(
		int pixelIndex,
		const Eigen::Vector3f &normal,
		const std::vector<Eigen::Vector3f> &normalizedLights,
		const LightKNN &knn) const;

private:
	void solveL2();
	void solveSBL();
	void solveRPCA();
	void solveRobust();
	void solveLambertian();
	void solveThree();

	// Core weighted L2 solver.
	// Builds the per-pixel linear system from lights and m_Row[p], optionally:
	//   - trimming the bottom trimLo fraction and top trimHi fraction of lights
	// Applies per-light soft weights from inWeights[m] ∈ [0, 1]
	// (nullptr = uniform weight 1 for every light) via weighted normal
	// equations: (A^T W A) x = A^T W b.
	// Returns the unit surface normal, or (0, 0, 1) when too few samples remain.
	Eigen::Vector3f solveL2Weighted(
		int p,
		const std::vector<Eigen::Vector3f> &lights,
		const float *inWeights = nullptr) const;

	// Core IRLS solver with selectable robust M-estimator.
	// Builds the per-pixel linear system from all lights and m_Row[p], then
	// iteratively reweights to down-weight outliers.
	// inWeights[m] ∈ [0, 1] seeds the initial weight vector for the first solve
	// (nullptr = start from uniform weights). Subsequent iterations use pure
	// residual weights from the chosen estimator.
	// useTukey = false → Huber (quadratic core, linear tails, convex)
	// useTukey = true  → Tukey bisquare (zero weight beyond cutoff, non-convex)
	// Returns the unit surface normal, or (0, 0, 1) when too few lights remain.
	Eigen::Vector3f solveRobustWeighted(
		int p,
		const std::vector<Eigen::Vector3f> &lights,
		const float *inWeights = nullptr) const;
private:
	NormalSolver solver;
	int row;
	PixelArray m_Row;

	Eigen::Vector3f* m_Normals;
	float*           m_WeightsOut = nullptr;
	ImageSet &m_Imageset;
	float robust_threshold_high;
	float robust_threshold_low;
	QMutex m_Mutex;
};

#endif // NORMALSWORKER_H
