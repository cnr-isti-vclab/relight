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

	crop = project.crop;
	//img_size = project.imgsize;

	imageset.initFromProject(project);
	imageset.setCrop(crop, project.offsets);
	imageset.rotateLights(-project.crop.angle);

	pixelSize = project.pixelSize;
}


void BrdfTask::initFromFolder(const char *folder, Dome &dome, Crop &crop) {
	imageset.initFromFolder(folder);
	imageset.initFromDome(dome);
	if(crop.width() > 0)
		imageset.setCrop(crop);
	imageset.rotateLights(-crop.angle);
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

	QDir destination(parameters.path);
	if(!destination.exists()) {
		if(!QDir().mkpath(parameters.path)) {
			error = "Could not create brdf folder.";
			status = FAILED;
			return;
		}
	}

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
			float* data = &albedo[idx];

			AlbedoWorker *task = new AlbedoWorker(parameters, i, line, data, imageset, lens);

			std::function<void(void)> run = [this, task](void)->void {
				task->run();
				delete task;
			};

			// Launch the task
			pool.queue(run);
			pool.waitForSpace();

			bool proceed = progressed("Computing albedo...", ((float)i / imageset.height) * 100);
			if(!proceed)
				return;
		}

		vector<uint8_t> albedomap(width * height * 3);
		for(size_t i = 0; i < albedo.size(); i++) {
				albedomap[i] = std::min(std::max(int(albedo[i]), 0), 255);
		}

		QImage img(albedomap.data(), width, height, width*3, QImage::Format_RGB888);
		if(crop.angle != 0.0f)
			img = crop.cropBoundingImage(img);

		// Set spatial resolution if known. Need to convert as pixelSize stored in mm/pixel whereas QImage requires pixels/m
		if( pixelSize > 0 ) {
			int dotsPerMeter = round(1000.0/pixelSize);
			img.setDotsPerMeterX(dotsPerMeter);
			img.setDotsPerMeterY(dotsPerMeter);
		}
		bool saved = img.save(destination.filePath(parameters.albedo_path), "jpg", parameters.quality);
		if(!saved) {
			status = FAILED;
			return;
		}
		progressed("Albedo done", 100);
	}
	status = DONE;
}
