#ifndef NORMALSTASK_H
#define NORMALSTASK_H

#endif // NORMALSTASK_H


#include <string>
#include <QJsonObject>
#include <QMutex>
#include <QRect>
#include "../src/relight_vector.h"
#include "../src/imageset.h"
#include "../src/project.h"
#include "task.h"
#include <QRunnable>

enum NormalSolver { NORMALS_L2, NORMALS_SBL, NORMALS_RPCA };
enum FlatMethod { NONE, RADIAL, FOURIER };

class NormalsTask :  public Task
{
public:
	NormalSolver solver;
	FlatMethod flatMethod;
	double flat_radius = 0.5;
	bool exportSurface = false;
	bool exportDepthmap = false;
	bool exportK = 2.0;
	QRect m_Crop;

	float pixelSize = 0.0f;

	NormalsTask(Project *_project, NormalSolver _solver, FlatMethod _flatMethod) :
		project(_project), solver(_solver), flatMethod(_flatMethod) {
	}

	virtual ~NormalsTask(){};
	virtual void run() override;

private:
	//TODO remove dependency on project!
	Project *project;

};

class NormalsWorker
{
public:
	NormalsWorker(NormalSolver _solver, int _row, PixelArray& toProcess, float* normals, ImageSet &imageset) :
		 solver(_solver), row(_row), m_Row(toProcess), m_Normals(normals), m_Imageset(imageset){}

	void run();
private:
	void solveL2();
	void solveSBL();
	void solveRPCA();

	NormalSolver solver;
	int row;
	PixelArray m_Row;
	float* m_Normals;
	ImageSet &m_Imageset;
	QMutex m_Mutex;
};
