#include "brdftask.h"
#include "../src/relight_threadpool.h"

#include <QImage>
#include <vector>
using namespace std;



void BrdfTask::initFromProject(Project &project) {
	lens = project.lens;
	imageset.width = imageset.image_width = project.lens.width;
	imageset.height = imageset.image_height = project.lens.height;

	//img_size = project.imgsize;

	imageset.initFromProject(project);
	parameters.crop = project.crop;
	imageset.setCrop(parameters.crop, project.offsets);
	imageset.rotateLights(-parameters.crop.angle);

	imageset.pixel_size = project.pixelSize;
}


void BrdfTask::initFromFolder(const char *folder, Dome &dome, const Crop &folderCrop) {
	imageset.initFromFolder(folder);
	imageset.initFromDome(dome);
	parameters.crop = folderCrop;
	if(folderCrop.width() > 0)
		imageset.setCrop(folderCrop);
	imageset.rotateLights(-folderCrop.angle);
}

void BrdfTask::setParameters(BrdfParameters &param) {
	parameters = param;
	label = parameters.summary();
}


void BrdfTask::run() {
	status = RUNNING;
	startedAt = QDateTime::currentDateTimeUtc();
	
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
		if(parameters.crop.angle != 0.0f)
			img = parameters.crop.cropBoundingImage(img);

		// Set spatial resolution if known. Need to convert as pixelSize stored in mm/pixel whereas QImage requires pixels/m
		if( imageset.pixel_size > 0 ) {
			int dotsPerMeter = round(1000.0/imageset.pixel_size);
			img.setDotsPerMeterX(dotsPerMeter);
			img.setDotsPerMeterY(dotsPerMeter);
		}
		bool saved = img.save(destination.filePath(parameters.albedo_path), "jpg", parameters.quality);
		if(!saved) {
			error = "Could not save the image: " + destination.filePath(parameters.albedo_path);
			status = FAILED;
			return;
		}
		progressed("Albedo done", 100);
	}
	status = DONE;
}

QJsonObject BrdfTask::info() const {
	QJsonObject obj = Task::info();
	obj["parameters"] = parameters.toJson();
	return obj;
}
