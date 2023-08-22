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

class NormalsTask :  public Task
{
public:

	enum FlatMethod { NONE, RADIAL, FOURIER };
	NormalsTask(QString& inputPath, QString& outputPath, QRect crop, uint32_t method, FlatMethod flatMethod, double flatRadius = 50) :
		m_InputFolder(inputPath), m_OutputFolder(outputPath), m_Crop(crop), m_Method(method), m_FlatMethod (flatMethod), m_FlatRadius(flatRadius) {}
    virtual ~NormalsTask(){};

    virtual void run() override;



public slots:
    bool progressed(std::string str, int percent) override;
private:
    QString m_InputFolder;
    QString m_OutputFolder;
    QRect m_Crop;
    uint32_t m_Method;
	FlatMethod m_FlatMethod;
	double m_FlatRadius;
};

class NormalsWorker
{
public:
	NormalsWorker(unsigned int method, PixelArray& toProcess, double* normals, std::vector<Vector3f> lights) :
         m_Method(method), m_Row(toProcess), m_Normals(normals), m_Lights(lights){}

    void run();
private:
    void solveL2();
    void solveSBL();
    void solveRPCA();
private:
    uint32_t m_Method;
    PixelArray m_Row;
	double* m_Normals;
    std::vector<Vector3f> m_Lights;
    QMutex m_Mutex;
};
