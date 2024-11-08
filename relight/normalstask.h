#ifndef NORMALSTASK_H
#define NORMALSTASK_H

#endif // NORMALSTASK_H


#include <string>
#include <QJsonObject>
#include <QMutex>
#include <QRect>
#include "../src/relight_vector.h"
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

    NormalsTask(QString& inputPath, QString& outputPath, QRect crop, NormalSolver _solver, FlatMethod _flatMethod) :
        solver(_solver), flatMethod(_flatMethod), m_Crop(crop) {
        input_folder = inputPath;
        output = outputPath;
    }
    virtual ~NormalsTask(){};
    virtual void run() override;

};

class NormalsWorker
{
public:
    NormalsWorker(NormalSolver _solver, PixelArray& toProcess, float* normals, std::vector<Vector3f> lights) :
         solver(_solver), m_Row(toProcess), m_Normals(normals), m_Lights(lights){}

    void run();
private:
    void solveL2();
    void solveSBL();
    void solveRPCA();
private:
    NormalSolver solver;
    PixelArray m_Row;

    float* m_Normals;
    std::vector<Vector3f> m_Lights;
    QMutex m_Mutex;
};
