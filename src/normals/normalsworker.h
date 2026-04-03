#ifndef NORMALSWORKER_H
#define NORMALSWORKER_H

#include "../imageset.h"
#include "normals_parameters.h"

#include <Eigen/Core>

#include <QMutex>

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
private:
	void solveL2();
	void solveSBL();
	void solveRPCA();
	void solveRobust();
private:
	NormalSolver solver;
	int row;
	PixelArray m_Row;

	Eigen::Vector3f* m_Normals;
	ImageSet &m_Imageset;
	float robust_threshold_high;
	float robust_threshold_low;
	QMutex m_Mutex;
};

#endif // NORMALSWORKER_H
