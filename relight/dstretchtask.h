#ifndef DSTRETCHTASK_H
#define DSTRETCHTASK_H

#include <Eigen/Eigen>
#include "../src/task.h"

class DStretchTask : public Task
{
public:
	DStretchTask(QObject *parent) : Task(parent) {}

	virtual ~DStretchTask(){}
	virtual void run() override;
};

#endif // DSTRETCHTASK_H
