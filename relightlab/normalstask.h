#ifndef NORMALSTASK_H
#define NORMALSTASK_H

#include "task.h"
#include "../src/project.h"
#include "../src/imageset.h"

#include <QJsonObject>
#include <QMutex>
#include <QRect>
#include <Eigen/Core>
#include <QRunnable>

enum NormalSolver { NORMALS_L2, NORMALS_SBL, NORMALS_RPCA };
enum FlatMethod { FLAT_NONE, FLAT_RADIAL, FLAT_FOURIER };
enum SurfaceIntegration { SURFACE_NONE, SURFACE_BNI, SURFACE_ASSM, SURFACE_FFT };

class NormalsParameters {
public:
	bool compute = true;
	QString input_path;

	NormalSolver solver = NORMALS_L2;

	FlatMethod flatMethod = FLAT_NONE;
	double flatPercentage = 20;


	SurfaceIntegration surface_integration = SURFACE_NONE;
	float bni_k = 2.0;
	float assm_error = 0.1;

	int quality = 95;
	QString path;

	QString summary();
};


class NormalsTask :  public Task {
public:
	NormalsParameters parameters;

	ImageSet imageset;
	Lens lens;
	float pixelSize = 0.0f;


	virtual void run() override;

	void setParameters(NormalsParameters &param);
	void initFromProject(Project &project);
	void assm(QString filename, std::vector<float> &normals, float precision);

};

class NormalsWorker
{
public:
	NormalsWorker(NormalSolver _solver, int _row, const PixelArray& toProcess, float* normals, ImageSet &imageset, Lens &_lens) :
		solver(_solver), row(_row), m_Row(toProcess), m_Normals(normals), m_Imageset(imageset), lens(_lens){
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
	Lens &lens;
	QMutex m_Mutex;
};



#endif // NORMALSTASK_H
