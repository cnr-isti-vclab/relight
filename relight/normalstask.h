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


class NormalsTask :  public Task
{
public:
    NormalSolver solver;
    bool exportSurface = false;
    bool exportK = 2.0;
    QRect m_Crop;

    NormalsTask(QString& inputPath, QString& outputPath, QRect crop, NormalSolver _solver) :
        m_Crop(crop), solver(_solver) {
        input_folder = inputPath;
        output = outputPath;
    }
    virtual ~NormalsTask(){};

    virtual void run() override;

public slots:
    bool progressed(std::string str, int percent) override;
private:


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
    //uint8_t* m_Normals;
    float* m_Normals;
    std::vector<Vector3f> m_Lights;
    QMutex m_Mutex;
};
