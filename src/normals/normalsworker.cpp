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
	}
}


void NormalsWorker::solveL2()
{
	vector<Vector3f> &m_Lights = m_Imageset.lights();

	// Pixel data
	Eigen::MatrixXd mLights(m_Lights.size(), 3);
	Eigen::MatrixXd mPixel(m_Lights.size(), 1);
	Eigen::MatrixXd mNormals;

	unsigned int normalIdx = 0;


	// Fill the lights matrix
	for (size_t i = 0; i < m_Lights.size(); i++)
		for (int j = 0; j < 3; j++)
			mLights(i, j) = m_Lights[i][j];

	// For each pixel in the line solve the system
	for (size_t p = 0; p < m_Row.size(); p++) {
		// Fill the pixel vector
		for (size_t m = 0; m < m_Lights.size(); m++)
			mPixel(m, 0) = m_Row[p][m].mean();

		if(m_Imageset.light3d) {
			for(size_t i = 0; i < m_Lights.size(); i++) {
				Vector3f light = m_Imageset.relativeLight(m_Lights[i], p, m_Imageset.height - row);
				light.normalize();
				for (int j = 0; j < 3; j++)
					mLights(i, j) = light[j];
			}
		}

		mNormals = (mLights.transpose() * mLights).ldlt().solve(mLights.transpose() * mPixel);
		mNormals.col(0).normalize();
		m_Normals[normalIdx] = Eigen::Vector3f(float(mNormals.col(0)[0]), float(mNormals.col(0)[1]), float(mNormals.col(0)[2]));

		normalIdx += 1;
	}
}

void NormalsWorker::solveSBL()
{
}

void NormalsWorker::solveRPCA()
{
}

// Robust least-squares solver using IRLS (Iteratively Reweighted Least Squares).
// Pixels with intensity above high_threshold are considered specular highlights and excluded.
// Pixels with intensity below low_threshold are considered in shadow and excluded.
// The remaining pixels are used in IRLS with a Huber-like weighting to further
// reduce the influence of remaining outliers.
void NormalsWorker::solveRobust()
{
	vector<Vector3f> &m_Lights = m_Imageset.lights();
	const size_t nLights = m_Lights.size();

	// Thresholds: pixel values in m_Row are Color3f::mean() – typically in [0, 255]
	const float high = robust_threshold_high;
	const float low  = robust_threshold_low;

	unsigned int normalIdx = 0;

	for (size_t p = 0; p < m_Row.size(); p++) {

		// Sort by |distance from nearest threshold| descending: worst violators at the back.
		// A sample below `low` has violation (low - val), above `high` has (val - high);
		// in-range samples get a negative "distance" so they sort towards the front.
		vector<pair<float, int>> samples;
		samples.reserve(nLights);
		for (size_t m = 0; m < nLights; m++) {
			float val = m_Row[p][m].mean();
			float violation = std::max(val - high, low - val); // >0 if out-of-range
			samples.push_back({ violation, (int)m });
		}
		std::sort(samples.begin(), samples.end());

		// Remove from the back while the worst sample is still out-of-range and
		// we have more than 3 samples left.
		int nValid = (int)nLights;
		while (nValid > 5 && samples[nValid - 1].first > 0.0f)
			nValid--;

		// Build A and b from the first nValid samples (already sorted best-first)
		MatrixXd A(nValid, 3);
		VectorXd b(nValid);

		for (int k = 0; k < nValid; k++) {
			int m = samples[k].second;
			Eigen::Vector3f light;
			if (m_Imageset.light3d) {
				light = m_Imageset.relativeLight(m_Lights[m], p, m_Imageset.height - row);
				light.normalize();
			} else {
				light = m_Lights[m];
			}
			A(k, 0) = light[0];
			A(k, 1) = light[1];
			A(k, 2) = light[2];
			b(k)    = m_Row[p][m].mean();
		}

		const int maxIter = 10;
		const double epsilon = 1e-6;  // numerical floor for weights
		const double huberDelta = 5.0; // Huber threshold in intensity units

		VectorXd weights = VectorXd::Ones(nValid);
		VectorXd x(3);


		for (int iter = 0; iter < maxIter; iter++) {
			// Weighted normal equations: (A^T W A) x = A^T W b
			MatrixXd AtW = A.transpose() * weights.asDiagonal();
			x = (AtW * A).ldlt().solve(AtW * b);

            if(nValid <= 5)
                break; // Not enough samples for robust estimation, skip IRLS

			// Compute residuals and update Huber weights
			VectorXd residuals = b - A * x;
			VectorXd newWeights(nValid);
			for (int k = 0; k < nValid; k++) {
				double r = std::abs(residuals(k));
				newWeights(k) = (r < huberDelta) ? 1.0 : (huberDelta / (r + epsilon));
			}

			// Check convergence
			if ((newWeights - weights).norm() < 1e-4)
				break;

			weights = newWeights;
		}

		// Normalise to get unit normal
		if (x.norm() > 1e-10)
			x.normalize();

		m_Normals[normalIdx] = Eigen::Vector3f(float(x(0)), float(x(1)), float(x(2)));
		normalIdx++;
	}
}
