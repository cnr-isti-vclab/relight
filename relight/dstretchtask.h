#ifndef DSTRETCHTASK_H
#define DSTRETCHTASK_H

#include <task.h>

class DStretchTask : public Task
{
public:
    DStretchTask(QObject *parent) : Task(parent) {}
    virtual ~DStretchTask(){}

    virtual void run() override;
    virtual void pause() override;
    virtual void stop() override;
};

#endif // DSTRETCHTASK_H
