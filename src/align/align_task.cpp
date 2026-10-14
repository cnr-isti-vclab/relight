#include "align_task.h"

#include "../project.h"

#include <QImageReader>

#include <algorithm>
#include <cmath>
#include <iostream>

using namespace std;

AlignTask::AlignTask(Project &project): project(project) {
	visible = false;
	owned = true;
	label = "Finding alignment candidates.";
}

void AlignTask::run() {
	setStatus(RUNNING);

	vector<int> used;
	for(size_t i = 0; i < project.images.size(); i++)
		if(!project.images[i].skip)
			used.push_back(int(i));
	image_indices = used;

	auto load = [&](int i, QRect region) {
		QImageReader reader(project.images[used[i]].filename);
		reader.setAutoTransform(false);
		reader.setClipRect(region);
		QImage image = reader.read();
		if(image.size() != region.size())
			image = project.readImage(used[i]).copy(region);
		return image;
	};
	function<bool(QString, int)> callback = [this](QString stage, int percent) { return progressed(stage, percent); };
	try {
		AutoAlign align(project.imgsize);
		correspondences = align.runRegion(selected_region, int(used.size()), load, &callback);
		translations = align.solveTranslations(int(used.size()), correspondences);
		vector<double> max_difference(translations.size(), 0.0);
		for(const AutoAlignCorrespondence &edge: correspondences) {
			if(edge.peaks.empty())
				continue;
			QPointF residual =
				translations[size_t(edge.second)] - translations[size_t(edge.first)] - edge.peaks.front().offset;
			double difference = hypot(residual.x(), residual.y());
			max_difference[size_t(edge.first)] = max(max_difference[size_t(edge.first)], difference);
			max_difference[size_t(edge.second)] = max(max_difference[size_t(edge.second)], difference);
		}
		cout << "Assisted alignment translations and max peak/system difference (pixels):" << endl;
		cout << "image\tdx\tdy\tmax_difference" << endl;
		for(size_t i = 0; i < translations.size(); i++)
			cout << image_indices[i] + 1 << '\t'
				 << translations[i].x() << '\t' << translations[i].y() << '\t'
				 << max_difference[i] << endl;
		for(AutoAlignCorrespondence &edge: correspondences) {
			edge.first = used[size_t(edge.first)];
			edge.second = used[size_t(edge.second)];
		}
	} catch(QString e) {
		if(status != STOPPED) {
			error = e;
			setStatus(FAILED);
		}
		return;
	}
	progressed("Done.", 100);
	setStatus(DONE);
}
