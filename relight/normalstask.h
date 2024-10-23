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

void saveNormals(NormalSolver, ImageSet &imageSet, QString output, std::function<bool(std::string s, int d)> *callback = nullptr );
void computeNormals(NormalSolver solver, ImageSet &imageSet, std::vector<float> &normals, std::function<bool(std::string s, int d)> *callback = nullptr);


class NormalsTask :  public Task {
public:
	NormalSolver solver;
	bool exportSurface = false;
	bool exportK = 2.0;
	QRect m_Crop;
	float pixelSize = 0;

	NormalsTask(Project *_project, NormalSolver _solver) :
		project(_project), solver(_solver){
	}
	virtual ~NormalsTask() {};

	virtual void run() override;

public slots:
	bool progressed(std::string str, int percent) override;

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
private:

	NormalSolver solver;
	int row;
	PixelArray m_Row;
	//uint8_t* m_Normals;
	float* m_Normals;
	ImageSet &m_Imageset;
	QMutex m_Mutex;
};
