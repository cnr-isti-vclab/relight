#include "brdftask.h"
#include "../src/relight_threadpool.h"

#include <QImage>
#include <vector>
using namespace std;


QString BrdfParameters::summary() {
	QString ret;
	if(albedo == MEDIAN)
		ret = "Median";
	return ret;
}


void BrdfTask::initFromProject(Project &project) {
	lens = project.lens;
	imageset.width = imageset.image_width = project.lens.width;
	imageset.height = imageset.image_height = project.lens.height;

	imageset.initFromProject(project);
	imageset.setCrop(project.crop, project.offsets);

	pixelSize = project.pixelSize;
}

void BrdfTask::setParameters(BrdfParameters &param) {
	parameters = param;
	label = parameters.summary();
}


void BrdfTask::run() {
	status = RUNNING;
	label = parameters.summary();

	function<bool(QString s, int d)> callback = [this](QString s, int n)->bool { return this->progressed(s, n); };

	vector<float> albedo;

	int width = 0, height = 0;

	if(parameters.albedo == BrdfParameters::MEDIAN) {
		width = imageset.width;
		height = imageset.height;

		albedo.resize(width * height * 3);
		RelightThreadPool pool;
		PixelArray line;
		imageset.setCallback(nullptr);
		pool.start(QThread::idealThreadCount());

		for (int i = 0; i < imageset.height; i++) {
			// Read a line
			imageset.readLine(line);

			// Create the normal task and get the run lambda
			uint32_t idx = i * 3 * imageset.width;
			//uint8_t* data = normals.data() + idx;
			float* data = &albedo[idx];

			MedianWorker *task = new MedianWorker(parameters, i, line, data, imageset, lens);

			std::function<void(void)> run = [this, task](void)->void {
				task->run();
				delete task;
			};

			// Launch the task
			pool.queue(run);
			pool.waitForSpace();

			bool proceed = progressed("Computing normals...", ((float)i / imageset.height) * 100);
			if(!proceed)
				return;
		}

		vector<uint8_t> albedomap(width * height * 3);
		for(size_t i = 0; i < albedomap.size(); i++)
			albedomap[i] = std::min(std::max(round(((albedo[i] + 1.0f) / 2.0f) * 255.0f), 0.0f), 255.0f);

		QImage img(albedomap.data(), width, height, width*3, QImage::Format_RGB888);


		// Set spatial resolution if known. Need to convert as pixelSize stored in mm/pixel whereas QImage requires pixels/m
		if( pixelSize > 0 ) {
			int dotsPerMeter = round(1000.0/pixelSize);
			img.setDotsPerMeterX(dotsPerMeter);
			img.setDotsPerMeterY(dotsPerMeter);
		}
		img.save(parameters.path, nullptr, 100);
	}
}
