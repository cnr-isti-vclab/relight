#include "normalstask.h"
#include "../src/jpeg_decoder.h"
#include "../src/jpeg_encoder.h"
#include "../src/imageset.h"
#include "../src/relight_threadpool.h"
#include "../src/bni_normal_integration.h"
#include "../src/flatnormals.h"

#include <Eigen/Eigen>
#include <Eigen/Core>
#include <Eigen/Dense>

#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
#include <QImage>
#include <vector>
#include <iostream>
#include <time.h>

//////////////////////////////////////////////////////// NORMALS TASK //////////////////////////////////////////////////////////
/// \brief NormalsTask: Takes care of creating the normals from the images given in a folder (inputFolder) and saves the file
///         in the outputFolder. After applying the crop described in the QRect passed as an argument to the constructor,
///         the NormalsTask creates a NormalsWorker for each line in the final image.
///         That NormalsWorker fills a vector with the colors of the normals in that line.
///

void NormalsTask::initFromProject(Project &project) {
	imageset.width = imageset.image_width = project.lens.width;
	imageset.height = imageset.image_height = project.lens.height;

	imageset.images = project.getImages();
	imageset.initImages(project.dir.absolutePath().toStdString().c_str());
	imageset.initLightsFromDome(project.dome);
	QRect &crop = project.crop;
	if(!crop.isNull()) {
		imageset.crop(crop.left(), crop.top(), crop.width(), crop.height());
	}
	pixelSize = project.pixelSize;
}

void NormalsTask::run() {
	status = RUNNING;


	imageset.setCallback(nullptr);

	std::vector<float> normals(imageset.width * imageset.height * 3);


	RelightThreadPool pool;
	PixelArray line;

	pool.start(QThread::idealThreadCount());

	for (int i=0; i<imageset.height; i++) {
		// Read a line
		imageset.readLine(line);

		// Create the normal task and get the run lambda
		uint32_t idx = i * 3 * imageset.width;
		//uint8_t* data = normals.data() + idx;
		float* data = &normals[idx];

		NormalsWorker *task = new NormalsWorker(solver, i, line, data, imageset);

		std::function<void(void)> run = [this, task](void)->void {
			task->run();
			delete task;
		};

		// Launch the task
		pool.queue(run);
		pool.waitForSpace();

		progressed("Computing normals...", ((float)i / imageset.height) * 100);
	}

	// Wait for the end of all the threads
	pool.finish();

	if(flatMethod != NONE) {
		//TODO: do we really need double precision?
		std::vector<double> normalsd(normals.size());
		for(uint32_t i = 0; i < normals.size(); i++)
			normalsd[i] = (double)normals[i];

		NormalsImage ni;
		ni.load(normalsd, imageset.width, imageset.height);
		switch(flatMethod) {
			case RADIAL:
				ni.flattenRadial();
				break;
			case FOURIER:
				//convert radius to frequencies
				double sigma = 100/m_FlatRadius;
				ni.flattenFourier(imageset.width/10, sigma);
				break;
		}
		normalsd = ni.normals;
		for(uint32_t i = 0; i < normals.size(); i++)
			normals[i] = (float)ni.normals[i];

	}


	std::vector<uint8_t> normalmap(imageset.width * imageset.height * 3);
	for(size_t i = 0; i < normals.size(); i++)
		normalmap[i] = floor(((normals[i] + 1.0f) / 2.0f) * 255);

	// Save the final result
	QImage img(normalmap.data(), imageset.width, imageset.height, imageset.width*3, QImage::Format_RGB888);
	img.save(output);

	// Set spatial resolution if known. Need to convert as pixelSize stored in mm/pixel whereas QImage requires pixels/m
	if( pixelSize > 0 ) {
		int dotsPerMeter = round(1000.0/pixelSize);
		img.setDotsPerMeterX(dotsPerMeter);
		img.setDotsPerMeterY(dotsPerMeter);
	}
	img.save(output);
	std::function<bool(QString s, int d)> callback = [this](QString s, int n)->bool { return this->progressed(s, n); };

	if(exportPly) {
		progressed("Integrating normals...", 0);
		std::vector<float> z;
		bni_integrate(callback, imageset.width, imageset.height, normals, z, bni_k);
		if(z.size() == 0) {
			error = "Failed to integrate normals";
			status = FAILED;
			return;
		}
		//TODO remove extension properly
		QString filename = output.left(output.size() -4) + ".ply";

		progressed("Saving surface...", 99);
		savePly(filename, imageset.width, imageset.height, z);
	}
	progressed("Done", 100);
}
/*
bool NormalsTask::progressed(QString str, int percent)
{
	if(status == PAUSED) {
		mutex.lock();  //mutex should be already locked. this talls the
		mutex.unlock();
	}
	if(status == STOPPED)
		return false;

	emit progress(str, percent);
	if(status == STOPPED)
		return false;
	return true;
}*/

/**
 *   \brief NormalsWorker: generates the normals for a given line in the image set, depending on the method specified when
 *          creating the Worker.
**/


void NormalsWorker::run()
{
	switch (solver)
	{
	// L2 solver
	case NORMALS_L2:
		solveL2();
		break;
		// SBL solver
	case NORMALS_SBL:
		solveSBL();
		break;
		// RPCA solver
	case NORMALS_RPCA:
		solveRPCA();
		break;
	}

	// Deallocate line (TODO: useless?)
	std::vector<Pixel>().swap(m_Row);
}


void NormalsWorker::solveL2()
{
	std::vector<Vector3f> &m_Lights = m_Imageset.lights;
	std::vector<Vector3f> &m_Lights3d = m_Imageset.lights3d;
	// Pixel data
	Eigen::MatrixXd mLights(m_Lights.size(), 3);
	Eigen::MatrixXd mPixel(m_Lights.size(), 1);
	Eigen::MatrixXd mNormals;

	unsigned int normalIdx = 0;


	// Fill the lights matrix
	for (size_t i = 0; i < m_Lights.size(); i++)
		for (int j = 0; j < 3; j++)
			mLights(i, j) = m_Lights[i][j];

	// For each pixel in the line solve the system
	//TODO do it in a single pass, it's faster.
	for (size_t p = 0; p < m_Row.size(); p++) {
		// Fill the pixel vector
		for (size_t m = 0; m < m_Lights.size(); m++)
			mPixel(m, 0) = m_Row[p][m].mean();

		if(m_Imageset.light3d) {
			for(size_t i = 0; i < m_Lights3d.size(); i++) {
				Vector3f light = m_Imageset.relativeLight(m_Lights3d[i], p, row);
				light.normalize();
				for (int j = 0; j < 3; j++)
					mLights(i, j) = light[j];
			}
		}



		mNormals = (mLights.transpose() * mLights).ldlt().solve(mLights.transpose() * mPixel);
		mNormals.col(0).normalize();
		m_Normals[normalIdx+0] = mNormals(0,0);
		m_Normals[normalIdx+1] = mNormals(1,0);
		m_Normals[normalIdx+2] = mNormals(2,0);

		normalIdx += 3;
	}
}
void NormalsWorker::solveSBL()
{
}

void NormalsWorker::solveRPCA()
{
}
