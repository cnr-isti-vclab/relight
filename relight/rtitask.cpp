#include "rtitask.h"

#include "../src/rti.h"
#include "../relight-cli/rtibuilder.h"

RtiTask::RtiTask()
{

}

RtiTask::~RtiTask() {
	if(builder)
		delete builder;

}
