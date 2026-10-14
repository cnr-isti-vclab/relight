#include "align_task.h"

#include "../image_decoder.h"
#include "../project.h"
#include "align.h"
#include "autoalign.h"

#include <QImage>

#include <algorithm>
#include <mutex>

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
			const int batch_size = 8;
			const int count = int(used.size());
			std::mutex thumbnail_mutex;
			std::vector<QString> errors(used.size());
			for(int start = 0; start < count; start += batch_size) {
				int end = std::min(count, start + batch_size);
				#pragma omp parallel for schedule(dynamic)
				for(int i = start; i < end; i++) {
					try {
						int index = used[size_t(i)];
						std::string filename =
							project.images[size_t(index)].filename.toStdString();
						int channels = 0;
						std::vector<uint8_t> tile_data;
						if(ImageDecoder::readRegion(filename.c_str(),
						                            align->rect.x(), align->rect.y(),
						                            align->rect.width(), align->rect.height(),
						                            tile_data, channels) != size_t(align->rect.height()))
							throw QString("Failed loading selected region from image: %1")
								.arg(project.images[size_t(index)].filename);
						if(channels != 1 && channels != 3 && channels != 4)
							throw QString("Unsupported number of channels in image: %1")
								.arg(channels);

						if(!cache_hit) {
							size_t tile_row_size =
								size_t(align->rect.width()) * size_t(channels);
							QImage::Format format = channels == 1
								? QImage::Format_Grayscale8
								: channels == 3 ? QImage::Format_RGB888
								                : QImage::Format_RGBA8888;
							QImage tile(tile_data.data(), align->rect.width(),
							            align->rect.height(), int(tile_row_size), format);
							std::lock_guard<std::mutex> lock(thumbnail_mutex);
							align->readThumb(tile, index);
						}
						if(update_positions)
							auto_align.provideImage(i, tile_data.data(), channels);
					} catch(QString e) {
						errors[size_t(i)] = e;
					}
				}

				for(int i = start; i < end; i++)
					if(!errors[size_t(i)].isEmpty())
						throw errors[size_t(i)];
				if(update_positions) {
					if(!progressed("Preparing selected regions", 50*end/count))
						return;
				} else if(!progressed("Loading alignment tiles", 99*end/count)) {
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
