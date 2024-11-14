#ifndef NORMALSTASK_H
#define NORMALSTASK_H

#include <QJsonObject>
#include <QMutex>
#include <QRect>
#include "../src/relight_vector.h"
#include "task.h"
#include "../src/project.h"
#include "../src/imageset.h"
#include <QRunnable>

enum NormalSolver { NORMALS_L2, NORMALS_SBL, NORMALS_RPCA };
enum FlatMethod { NONE, RADIAL, FOURIER };


class NormalsTask :  public Task {
public:
	NormalSolver solver = NORMALS_L2;
	FlatMethod flatMethod = NONE;
	double m_FlatRadius = 0.5;

	bool exportJpeg = true;
	int quality = 95;
	bool exportPng = false;
	bool exportTiff = false;

	bool exportPly = false;
	bool bni_k = 2.0;
	ImageSet imageset;
	float pixelSize = 0.0f;

	NormalsTask(const QString &outputPath) {
		output = outputPath;
	}

	virtual ~NormalsTask(){};
	void initFromProject(Project &project);
	virtual void run() override;
};

class NormalsWorker
{
public:
	NormalsWorker(NormalSolver _solver, int _row, const PixelArray& toProcess, float* normals, ImageSet &imageset) :
		solver(_solver), row(_row), m_Row(toProcess), m_Normals(normals), m_Imageset(imageset){
		m_Row.resize(toProcess.npixels(), toProcess.nlights);
		for(size_t i = 0; i < m_Row.size(); i++)
			m_Row[i] = toProcess[i];
	}

	void run();
private:
	void solveL2();
	void solveSBL();
	void solveRPCA();
private:

	NormalSolver solver;
	int row;
	PixelArray m_Row;

	float* m_Normals;
	ImageSet &m_Imageset;
	QMutex m_Mutex;
};



#endif // NORMALSTASK_H
