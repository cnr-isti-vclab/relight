#ifndef DSTRETCHTASK_H
#define DSTRETCHTASK_H

#include <Eigen/Eigen>
#include <task.h>

class DStretchTask : public Task
{
public:
	DStretchTask(QObject *parent) : Task(parent) {}
    virtual ~DStretchTask(){}

    virtual void run() override;
	virtual bool progressed(QString str, int percent) override;

private:
};

#endif // DSTRETCHTASK_H
