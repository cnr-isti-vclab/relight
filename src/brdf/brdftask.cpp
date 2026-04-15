#include "brdftask.h"
#include "../src/relight_threadpool.h"
#include "../src/jpeg_encoder.h"
#include "../src/icc_profiles.h"

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

	imageset.pixel_size = project.pixelSize();
	imageset.setColorProfileMode(COLOR_PROFILE_LINEAR_RGB);
	imageset.createOutputColorTransform(COLOR_PROFILE_SRGB);
}


void BrdfTask::initFromFolder(const char *folder, Dome &dome, const Crop &folderCrop) {
	imageset.initFromFolder(folder);
	imageset.initFromDome(dome);
	parameters.crop = folderCrop;
	if(folderCrop.width() > 0)
		imageset.setCrop(folderCrop);
	imageset.rotateLights(-folderCrop.angle);
	imageset.setColorProfileMode(COLOR_PROFILE_LINEAR_RGB);
	imageset.createOutputColorTransform(COLOR_PROFILE_SRGB);
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

	int width = 0, height = 0;

	QDir destination(parameters.path);
	if(!destination.exists()) {
		if(!QDir().mkpath(parameters.path)) {
			error = "Could not create brdf folder.";
			status = FAILED;
			return;
		}
	}

	if(parameters.albedo != BrdfParameters::NONE) {
		width = imageset.width;
		height = imageset.height;

		vector<uint8_t> albedomap(width * height * 3);
		RelightThreadPool pool;
		PixelArray line;
		imageset.setCallback(nullptr);
		pool.start(QThread::idealThreadCount());
		for (int i = 0; i < imageset.height; i++) {
			// Read a line
			imageset.readLine(line);

			// Create the normal task and get the run lambda
			uint32_t idx = i * 3 * imageset.width;
			uint8_t* data = &albedomap[idx];

			AlbedoWorker *task = new AlbedoWorker(parameters, i, line,
			                                      imageset.output_color_transform_float, data, imageset, lens);

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
		pool.finish();

		// Handle optional rotated crop via QImage, then encode with JpegEncoder
		// so we can embed the correct output ICC profile.
		uint8_t *pixels = albedomap.data();
		int out_w = width, out_h = height;
		QImage rotated; // kept alive until encode
		if(parameters.crop.angle != 0.0f) {
			QImage tmp(albedomap.data(), width, height, width*3, QImage::Format_RGB888);
			rotated = parameters.crop.cropBoundingImage(tmp).convertToFormat(QImage::Format_RGB888);
			pixels = rotated.bits();
			out_w  = rotated.width();
			out_h  = rotated.height();
		}

		JpegEncoder encoder;
		encoder.setColorSpace(JCS_RGB, 3);
		encoder.setJpegColorSpace(JCS_RGB);
		encoder.setQuality(parameters.quality);
		encoder.setChromaSubsampling(false);
		//if(imageset.pixel_size > 0)
		//	encoder.setDotsPerMeter(round(1000.0 / imageset.pixel_size));
		const auto icc = imageset.getOutputICCProfile(COLOR_PROFILE_SRGB);
		if(!icc.empty())
			encoder.setICCProfile(icc);

		QString albedoPath = destination.filePath(parameters.albedo_path + ".jpg");
		if(!encoder.encode(pixels, out_w, out_h, albedoPath.toStdString().c_str())) {
			error = "Could not save the image: " + albedoPath;
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
