#ifndef ALIGN_TASK_H
#define ALIGN_TASK_H

#include "../task.h"

class Project;
class Align;

class AlignTask: public Task {
public:
	Align *align;
	bool update_positions = true;

	AlignTask(Project &project, Align *align);
	void run() override;

private:
	Project &project;
};

#endif // ALIGN_TASK_H
