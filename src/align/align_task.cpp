#include "align_task.h"

#include "../project.h"
#include "align.h"
#include "autoalign.h"

#include <QImage>

#include <algorithm>

AlignTask::AlignTask(Project &project, Align *align): align(align), project(project) {
	visible = false;
	owned = true;
	label = "Finding image alignment.";
}

void AlignTask::run() {
	setStatus(RUNNING);

	align->thumbs.resize(project.images.size());
	QImage cached_thumbnails;
	bool cache_hit = cached_thumbnails.load(align->cacheFilename(), "JPG")
		&& align->readCacheThumbs(cached_thumbnails);

	std::vector<int> used;
	for(size_t i = 0; i < project.images.size(); i++)
		if(!project.images[i].skip)
			used.push_back(int(i));

	try {
		std::function<bool(QString, int)> callback =
			[this](QString stage, int percent) { return progressed(stage, std::min(percent, 99)); };
		AutoAlign auto_align(project.imgsize);
		if(update_positions) {
			if(used.size() < 2)
				throw QString("At least two images are required to align.");
			auto_align.beginRegion(align->rect.size(), int(used.size()));
		}

		if(!cache_hit || update_positions) {
			for(size_t i = 0; i < used.size(); i++) {
				int index = used[i];
				QImage image = project.readImage(index);
				if(image.isNull())
					throw QString("Failed loading image: %1").arg(project.images[size_t(index)].filename);
				QImage tile = image.copy(align->rect);
				if(!cache_hit)
					align->readThumb(tile, index);
				if(update_positions) {
					auto_align.provideImage(int(i), tile);
					if(!progressed("Preparing selected regions", 50*int(i + 1)/int(used.size())))
						return;
				} else if(!progressed("Loading alignment tiles", int(99*(i + 1)/used.size()))) {
					return;
				}
			}
		}

		if(!cache_hit)
			align->saveCacheThumbs(align->cacheFilename());

		if(!update_positions) {
			progressed("Done.", 100);
			setStatus(DONE);
			return;
		}

		std::vector<AutoAlignCorrespondence> correspondences =
			auto_align.findCandidates(&callback);

		if(status == STOPPED)
			return;

		if(!progressed("Solving image translations", 99))
			return;
		std::vector<Eigen::Vector2f> translations =
			auto_align.solveTranslations(int(used.size()), correspondences);
		for(size_t i = 0; i < used.size(); i++) {
			const Eigen::Vector2f &translation = translations[i];
			align->offsets[size_t(used[i])] = QPointF(translation.x(), translation.y());
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
