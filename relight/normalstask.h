#ifndef NORMALSTASK_H
#define NORMALSTASK_H

#endif // NORMALSTASK_H

#include <vector.h>
#include <string>
#include <QJsonObject>
#include <QMutex>
#include "vector.h"
#include "task.h"
#include <QRunnable>

class NormalsTask : public QRunnable, public Task
{
public:
    NormalsTask(unsigned int method, PixelArray& toProcess, uint8_t* normals, std::vector<Vector3f> lights) :
         m_Method(method), m_Row(toProcess), m_Normals(normals), m_Lights(lights){}
    virtual ~NormalsTask(){};

    virtual void run() override;
    virtual void resume() override {};
    virtual void pause() override {};
    virtual void stop() override {};

public slots:
    bool progressed(std::string str, int percent) override;
private:
    void solveL2();
    void solveSBL();
    void solveRPCA();
private:
    unsigned int m_Method;
    PixelArray m_Row;
    uint8_t* m_Normals;
    std::vector<Vector3f> m_Lights;
    QMutex m_Mutex;
};
