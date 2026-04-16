#include "normalsworker.h"

#include <Eigen/Dense>

#include <vector>
#include <cmath>
#include <algorithm>

using namespace std;
using namespace Eigen;


void NormalsWorker::run() {
	switch (solver)
	{
	case NORMALS_L2:
		solveL2();
		break;
	case NORMALS_SBL:
		solveSBL();
		break;
	case NORMALS_RPCA:
		solveRPCA();
		break;
	case NORMALS_ROBUST:
		solveRobust();
		break;
	case NORMALS_LAMBERTIAN:
		solveLambertian();
		break;
	}
}


void NormalsWorker::solveL2()
{
	vector<Vector3f> m_Lights = m_Imageset.lights();
	for (Vector3f &l : m_Lights)
		l.normalize();
	const int nLights = (int)m_Lights.size();

	unsigned int normalIdx = 0;

	for (size_t p = 0; p < m_Row.size(); p++) {
		m_Normals[normalIdx] = solveL2Trimmed((int)p, m_Lights);
		normalIdx++;

		if (m_WeightsOut) {
			float* w = m_WeightsOut + p * nLights;
			for (int m = 0; m < nLights; m++)
				w[m] = 1.0f;
		}
	}
}

void NormalsWorker::solveSBL()
{
}

void NormalsWorker::solveRPCA()
{
}

// Robust IRLS normal solver.
// Applies Huber reweighting iteratively to reduce outlier influence.
// inWeights[m] ∈ [0, 1] seeds the first solve as the starting weight vector
// (nullptr = start from uniform weights). From the second iteration onward,
// only the Huber residual weights drive the reweighting.
// Returns the unit surface normal, or (0, 0, 1) when too few lights remain.
Eigen::Vector3f NormalsWorker::solveRobustWeighted(
	int p,
	const std::vector<Eigen::Vector3f> &lights,
	const float *inWeights) const
{
	const int nL = (int)lights.size();
	if (nL < 5)
		return Eigen::Vector3f(0.0f, 0.0f, 1.0f);

	MatrixXd A(nL, 3);
	VectorXd b(nL);

	for (int m = 0; m < nL; m++) {
		Eigen::Vector3f light;
		if (m_Imageset.light3d) {
			light = m_Imageset.relativeLight(lights[m], p, m_Imageset.height - row);
			light.normalize();
		} else {
			light = lights[m];
		}
		A(m, 0) = light[0];
		A(m, 1) = light[1];
		A(m, 2) = light[2];
		b(m)    = m_Row[p][m].mean();
	}

	const int    maxIter   = 10;
	const double epsilon   = 1e-6;
	const double huberDelta = 5.0;

	VectorXd weights(nL);
	for (int m = 0; m < nL; m++)
		weights(m) = inWeights ? (double)inWeights[m] : 1.0;

	VectorXd x(3);
	for (int iter = 0; iter < maxIter; iter++) {
		MatrixXd AtW = A.transpose() * weights.asDiagonal();
		x = (AtW * A).ldlt().solve(AtW * b);

		// Compute Huber-reweighted update (pure Huber — initial weights only seeded iter 0)
		VectorXd residuals = b - A * x;
		VectorXd newWeights(nL);
		for (int m = 0; m < nL; m++) {
			double r  = std::abs(residuals(m));
			newWeights(m) = (r < huberDelta) ? 1.0 : (huberDelta / (r + epsilon));
		}

		if ((newWeights - weights).norm() < 1e-4)
			break;

		weights = newWeights;
	}

	if (x.norm() < 1e-10)
		return Eigen::Vector3f(0.0f, 0.0f, 1.0f);
	x.normalize();
	return Eigen::Vector3f(float(x(0)), float(x(1)), float(x(2)));
}

void NormalsWorker::solveRobust()
{
	vector<Vector3f> &m_Lights = m_Imageset.lights();
	const size_t nLights = m_Lights.size();

	unsigned int normalIdx = 0;
	for (size_t p = 0; p < m_Row.size(); p++) {
		m_Normals[normalIdx++] = solveRobustWeighted((int)p, m_Lights);

		if (m_WeightsOut) {
			float* w = m_WeightsOut + p * nLights;
			for (int m = 0; m < (int)nLights; m++) w[m] = 1.0f;
		}
	}
}

// ---- Shadow / highlight detector ----------------------------------------

// Build a k-nearest-neighbour graph on pre-normalised light directions.
// Each entry knn[i][m] = {proximity_weight, j} where
// proximity_weight = dot(light_i, light_j) ∈ (0, 1].
// Closer lights (higher dot product) get a larger weight.
LightKNN NormalsWorker::buildLightKNN(
		const std::vector<Eigen::Vector3f> &lights, int k)
{
	int nL = (int)lights.size();
	k = std::min(k, nL - 1);
	LightKNN knn(nL);
	for (int i = 0; i < nL; i++) {
		// Sort by ascending -dot so partial_sort gives the closest first.
		std::vector<std::pair<float, int>> dists;
		dists.reserve(nL - 1);
		for (int j = 0; j < nL; j++) {
			if (i == j) continue;
			float dot = lights[i].dot(lights[j]);
			dists.push_back({ -dot, j });
		}
		std::partial_sort(dists.begin(), dists.begin() + k, dists.end());
		knn[i].resize(k);
		for (int m = 0; m < k; m++)
			knn[i][m] = { -dists[m].first, dists[m].second };
	}
	return knn;
}

// Core weighted L2 normal solver.
//
// Builds the per-pixel linear system from all lights and m_Row[p].
// Per-light soft weights from inWeights[m] ∈ [0, 1] (nullptr = all 1) are
// applied via weighted normal equations: (A^T W A) x = A^T W b.
// Returns the unit surface normal, or (0, 0, 1) when fewer than 5 lights
// have non-zero weight.
Eigen::Vector3f NormalsWorker::solveL2Weighted(
	int p,
	const std::vector<Eigen::Vector3f> &lights,
	const float *inWeights) const {

	const int nL = (int)lights.size();
	if (nL < 5)
		return Eigen::Vector3f(0.0f, 0.0f, 1.0f);

	MatrixXd A(nL, 3);
	VectorXd b(nL);
	VectorXd wvec(nL);

	for (int m = 0; m < nL; m++) {
		Eigen::Vector3f light;
		if (m_Imageset.light3d) {
			light = m_Imageset.relativeLight(lights[m], p, m_Imageset.height - row);
			light.normalize();
		} else {
			light = lights[m];
		}
		A(m, 0) = light[0];
		A(m, 1) = light[1];
		A(m, 2) = light[2];
		b(m)    = m_Row[p][m].mean();
		wvec(m) = inWeights ? (double)inWeights[m] : 1.0;
	}

	VectorXd x;
	if (!inWeights) {
		x = (A.transpose() * A).ldlt().solve(A.transpose() * b);
	} else {
		MatrixXd AtW = A.transpose() * wvec.asDiagonal();
		x = (AtW * A).ldlt().solve(AtW * b);
	}

	if (x.norm() < 1e-10)
		return Eigen::Vector3f(0.0f, 0.0f, 1.0f);
	x.normalize();
	return Eigen::Vector3f(float(x(0)), float(x(1)), float(x(2)));
}

// Trimmed L2 normal estimate: fit using only the middle 60% of lights
// (by intensity) to skip obvious shadows (dark) and highlights (bright).
// Computes binary trim weights (0 for bottom/top 20%, 1 for middle 60%),
// then delegates to solveL2Weighted. Falls back to (0,0,1) if too few
// lights have non-zero weight.
Eigen::Vector3f NormalsWorker::solveL2Trimmed(
		int p,
		const std::vector<Eigen::Vector3f> &lights) const {

	const int nL = (int)lights.size();

	// Sort lights by raw pixel intensity to find the trim boundaries.
	std::vector<std::pair<float, int>> sorted(nL);
	for (int m = 0; m < nL; m++)
		sorted[m] = { m_Row[p][m].mean(), m };
	std::sort(sorted.begin(), sorted.end());

	const int lo = (int)(nL * 0.2f);
	const int hi = nL - (int)(nL * 0.2f);

	// Binary weights: 1 for the middle 60%, 0 for the trimmed extremes.
	std::vector<float> weights(nL, 0.0f);
	for (int k = lo; k < hi; k++)
		weights[sorted[k].second] = 1.0f;

	return solveL2Weighted(p, lights, weights.data());
}

Eigen::VectorXf NormalsWorker::computeShadowMask(
		int p,
		const Eigen::Vector3f &normal,
		const std::vector<Eigen::Vector3f> &lights,
		const LightKNN &knn) const {
	using namespace Eigen;
	const int nL = (int)lights.size();

	VectorXf mask = VectorXf::Zero(nL);
	if (nL < 3) return mask;

	const float eps = 1e-6f;

	// Step 1 – Expected Lambertian cosines: λ[m] = max(0, l_m · normal).
	std::vector<float> lambda(nL);
	for (int m = 0; m < nL; m++)
		lambda[m] = std::max(0.0f, lights[m].dot(normal));

	// Step 2 – Measured luminance and normalised albedo estimates.
	// nval[m] = val[m] / λ[m]: Lambertian-consistent lights cluster near the
	// true albedo; shadows give nval << albedo, highlights give nval >> albedo.
	// -1 marks unusable entries (below horizon or near-zero value).
	std::vector<float> val(nL);
	for (int m = 0; m < nL; m++)
		val[m] = m_Row[p][m].mean();

	std::vector<float> nval(nL, -1.0f);
	for (int m = 0; m < nL; m++)
		if (lambda[m] > eps && val[m] > eps)
			nval[m] = val[m] / lambda[m];

	// Step 3 – Greedy consistent-subset search.
	// Initialise active set with all usable lights.
	std::vector<bool> active(nL, false);
	int activeCount = 0;
	for (int m = 0; m < nL; m++)
		if (nval[m] > 0.0f) { active[m] = true; ++activeCount; }

#define REMOVAL
#ifdef REMOVAL

	// Iteratively remove the most internally-inconsistent light until all
	// remaining KNN edges have a log-ratio below log_threshold, keeping at
	// least min_active lights.
	const float log_threshold = 0.5f;  // ≈ 65 % ratio difference
	const int   min_active    =  3;

	std::vector<int> removal_step(nL, 0);  // 0 = survived into the final subset
	int step = 0;

	while (activeCount > min_active) {
		float max_score = 0.0f;
		int   worst     = -1;

		for (int i = 0; i < nL; i++) {
			if (!active[i]) continue;
			float wsum = 0.0f, wtot = 0.0f;
			for (const auto &[w, j] : knn[i]) {
				if (!active[j]) continue;
				wsum += w * std::abs(std::log(nval[i] / nval[j]));
				wtot += w;
			}
			if (wtot == 0.0f) continue;
			float score = wsum / wtot;
			if (score > max_score) { max_score = score; worst = i; }
		}

		if (max_score < log_threshold || worst == -1) break;

		active[worst] = false;
		removal_step[worst] = ++step;
		--activeCount;
	}

	// If fewer than 50% of the originally usable lights survived, the removal
	// was too aggressive. Fall back: mask only the raw-intensity top and bottom
	// 20% and treat everything else as Lambertian.
	const int originalActiveCount = activeCount + step;
	if (activeCount * 3 < originalActiveCount) {
		mask = VectorXf::Zero(nL);
		std::vector<std::pair<float, int>> byVal(nL);
		for (int m = 0; m < nL; m++) byVal[m] = { val[m], m };
		std::sort(byVal.begin(), byVal.end());
		int cut = nL / 5;
		for (int k = 0;        k < cut; k++) mask(byVal[k].second)      = 1.0f;
		for (int k = nL - cut; k < nL;  k++) mask(byVal[k].second)      = 1.0f;
		return mask;
	}

	// Step 4 – Convert removal order to a probability in [0, 1].
	//   Removed first (step 1) = most inconsistent → prob 1.
	//   Removed last  (step N) = least inconsistent removed → prob 1/N.
	//   Lights in the final consistent subset → prob 0.
	//   Lights that were unusable from the start → prob 1.
	if (step > 0) {
		const float inv_step = 1.0f / float(step);
		for (int m = 0; m < nL; m++)
			if (removal_step[m] > 0)
				mask(m) = float(step - removal_step[m] + 1) * inv_step;
	}
	for (int m = 0; m < nL; m++)
		if (nval[m] < 0.0f) mask(m) = 1.0f;
#else

	// Region-growing segmentation on the KNN graph.
	//
	// Seed selection: sort valid lights by nval and pick the one at the 60th
	// percentile.  This sits above most shadows (low nval) and below most
	// specular highlights (high nval), so it is very likely Lambertian.
	//
	// Growing: propagate through KNN edges, adding a neighbour whenever its
	// nval is within log_threshold of the current region mean.  The region
	// mean is updated incrementally as nodes are added (BFS order, walking
	// the vector by index so no extra header is needed).
	//
	// Result:
	//   In-region lights  → mask = 0      (Lambertian-consistent)
	//   Out-of-region     → mask ∝ log-ratio distance from region mean
	//   Unusable (nval<0) → mask = 1

	const float log_threshold = 0.5f;   // ≈ 65 % ratio gap

	// Collect valid nval indices and sort by nval for percentile selection.
	std::vector<int> validIdx;
	validIdx.reserve(activeCount);
	for (int m = 0; m < nL; m++)
		if (nval[m] > 0.0f) validIdx.push_back(m);

	if (validIdx.empty()) {
		for (int m = 0; m < nL; m++) mask(m) = 1.0f;
		return mask;
	}
	std::sort(validIdx.begin(), validIdx.end(),
	          [&](int a, int b){ return nval[a] < nval[b]; });

	// Seed at the 60th percentile.
	int seed = validIdx[(int)(validIdx.size() * 60 / 100)];

	// BFS region growing via index-walking vector (avoids std::queue).
	std::vector<bool> inRegion(nL, false);
	std::vector<int>  region;
	region.reserve(nL);
	region.push_back(seed);
	inRegion[seed] = true;
	double regionSum = nval[seed];
	int    regionCnt = 1;

	for (size_t ri = 0; ri < region.size(); ri++) {
		int cur = region[ri];
		float regionMean = (float)(regionSum / regionCnt);
		for (const auto &[w, j] : knn[cur]) {
			if (!active[j] || inRegion[j]) continue;
			if (std::abs(std::log(nval[j] / regionMean)) < log_threshold) {
				inRegion[j] = true;
				region.push_back(j);
				regionSum += nval[j];
				regionCnt++;
			}
		}
	}

	// Convert region membership to a continuous probability in [0, 1].
	float regionMean = (float)(regionSum / regionCnt);
	for (int m = 0; m < nL; m++) {
		if (nval[m] < 0.0f) {
			mask(m) = 1.0f;                           // unusable (below horizon / black pixel)
		} else if (inRegion[m]) {
			mask(m) = 0.0f;                           // Lambertian-consistent
		} else {
			float logRatio = std::abs(std::log(nval[m] / regionMean));
			mask(m) = std::min(1.0f, logRatio / log_threshold);
		}
	}

#endif

	return mask;
}

// Lambertian solver: build the KNN graph once for the whole row, then for
// each pixel compute the shadow mask using its own normal and solve a
// weighted least-squares system where each light's weight = 1 - shadow_prob.
// This down-weights shadows and highlights without requiring an explicit
// threshold, and is equivalent to a single-pass weighted L2 solve with
// data-driven weights.
void NormalsWorker::solveLambertian() {
	vector<Vector3f> m_Lights = m_Imageset.lights();
	for(Vector3f &l: m_Lights)
		l.normalize();
	const int nLights = (int)m_Lights.size();

	// Build the KNN graph once; reuse it for every pixel in this row.
	const LightKNN knn = buildLightKNN(m_Lights);

	unsigned int normalIdx = 0;

	for (size_t p = 0; p < m_Row.size(); p++) {

		// Rough normal via trimmed L2 (middle 60% of intensities).
		const Eigen::Vector3f pixelNormal = solveL2Trimmed((int)p, m_Lights);

		// Compute shadow probabilities using the per-pixel rough normal.
		const Eigen::VectorXf shadowWeights = computeShadowMask((int)p, pixelNormal, m_Lights, knn);

		// Per-light weights: w_m = 1 - shadow_prob.
		// A pixel in full shadow (prob=1) gets weight 0 and is effectively excluded.
		std::vector<float> wf(nLights);
		for (int m = 0; m < nLights; m++)
			wf[m] = 1.0f - shadowWeights(m);

		m_Normals[normalIdx] = solveRobustWeighted((int)p, m_Lights, wf.data());
		normalIdx++;

		if (m_WeightsOut) {
			float* w = m_WeightsOut + p * nLights;
			for (int m = 0; m < nLights; m++)
				w[m] = wf[m];
		}
	}
}
