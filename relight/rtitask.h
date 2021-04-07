#ifndef RTITASK_H
#define RTITASK_H

#include "task.h"

class RtiBuilder;

class RtiTask: public Task {
	Q_OBJECT
public:

	RtiTask();
	virtual ~RtiTask();
	virtual void run() override {};
	virtual void pause() override {};
	virtual void resume() override {};
	virtual void stop() override {};

private:
	RtiBuilder *builder = nullptr;

};

#endif // RTITASK_H
