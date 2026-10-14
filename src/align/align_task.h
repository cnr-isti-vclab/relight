#ifndef ALIGN_TASK_H
#define ALIGN_TASK_H

#include "autoalign.h"
#include "../task.h"

#include <Eigen/Core>
#include <QRect>
#include <vector>

class Project;

class AlignTask: public Task {
public:
	QRect selected_region;
	std::vector<int> image_indices;
	std::vector<AutoAlignCorrespondence> correspondences;
	std::vector<Eigen::Vector2f> translations;

	explicit AlignTask(Project &project);
	void run() override;

private:
	Project &project;
};

#endif // ALIGN_TASK_H
