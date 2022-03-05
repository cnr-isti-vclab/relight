#ifndef NORMALSTASK_H
#define NORMALSTASK_H

#endif // NORMALSTASK_H

#include <vector.h>
#include <string>
#include <QJsonObject>
#include "task.h"

class NormalsTask : public Task
{
public:
    NormalsTask(const QString& inputDir, const QString& outputDir, unsigned int method, QJsonObject crop) :
        m_InputDir(inputDir), m_OutputDir(outputDir),  m_Crop(crop), m_Method(method){}
    virtual ~NormalsTask(){};

    virtual void run() override;
    virtual void pause() override;
    virtual void resume() override;
    virtual void stop() override;

public slots:
    bool progressed(std::string str, int percent) override;
private:
    void solveL2();
    void solveSBL();
    void solveRPCA();
private:
    const QString m_InputDir;
    const QString m_OutputDir;
    const QJsonObject m_Crop;
    unsigned int m_Method;
};
