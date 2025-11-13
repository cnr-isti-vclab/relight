#ifndef NORMALSTASK_H
#define NORMALSTASK_H

#include "../task.h"
#include "../project.h"
#include "../imageset.h"
#include "normals_parameters.h"

#include <QJsonObject>
#include <QMutex>
#include <QRect>
#include <Eigen/Core>
#include <QRunnable>

class NormalsTask :  public Task {
public:
	NormalsParameters parameters;

	ImageSet imageset;
	Crop crop;
	Lens lens;
	float z_threshold =0.001;

	virtual void run() override;

	void setParameters(NormalsParameters &param);
	void initFromProject(Project &project);
	void initFromFolder(const char *folder, Dome &dome, Crop &crop);

	void assm(QString filename, std::vector<Eigen::Vector3f> &normals, int width, int height, float precision);
	void fixNormal(Eigen::Vector3f &n); //check for nan, and z< threshold
};

class NormalsWorker
{
public:
	NormalsWorker(NormalSolver _solver, int _row, const PixelArray& toProcess, Eigen::Vector3f* normals, ImageSet &imageset): //, Lens &_lens) :
		solver(_solver), row(_row), m_Row(toProcess), m_Normals(normals), m_Imageset(imageset) {//lens(_lens){
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

	Eigen::Vector3f* m_Normals;
	ImageSet &m_Imageset;
	//Lens &lens;
	QMutex m_Mutex;
};



#endif // NORMALSTASK_H
