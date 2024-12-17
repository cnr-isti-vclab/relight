#include "normalstask.h"
#include "../src/jpeg_decoder.h"
#include "../src/jpeg_encoder.h"
#include "../src/imageset.h"
#include "../src/relight_threadpool.h"
#include "../src/bni_normal_integration.h"
#include "../src/fft_normal_integration.h"
#include "../src/flatnormals.h"

#include <assm/Grid.h>
#include <assm/algorithms/PhotometricRemeshing.h>
#include <assm/algorithms/Integration.h>

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

using namespace std;
using namespace Eigen;

//////////////////////////////////////////////////////// NORMALS TASK //////////////////////////////////////////////////////////
/// \brief NormalsTask: Takes care of creating the normals from the images given in a folder (inputFolder) and saves the file
///         in the outputFolder. After applying the crop described in the QRect passed as an argument to the constructor,
///         the NormalsTask creates a NormalsWorker for each line in the final image.
///         That NormalsWorker fills a vector with the colors of the normals in that line.
///

void NormalsTask::initFromProject(Project &project) {
	lens = project.lens;
	imageset.width = imageset.image_width = project.lens.width;
	imageset.height = imageset.image_height = project.lens.height;

	imageset.images = project.getImages();
	imageset.initImages(project.dir.absolutePath().toStdString().c_str());
	imageset.initFromDome(project.dome);
	assert(imageset.lights().size() == imageset.images.size());
	QRect &crop = project.crop;
	if(!crop.isNull()) {
		imageset.crop(crop.left(), crop.top(), crop.width(), crop.height());
	}
	pixelSize = project.pixelSize;
}

void NormalsTask::run() {
	status = RUNNING;


	function<bool(QString s, int d)> callback = [this](QString s, int n)->bool { return this->progressed(s, n); };

	vector<float> normals;

	int width = 0, height = 0;

	if(parameters.compute) {
		width = imageset.width;
		height = imageset.height;

		normals.resize(imageset.width * imageset.height * 3);
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
			float* data = &normals[idx];

			NormalsWorker *task = new NormalsWorker(parameters.solver, i, line, data, imageset, lens);

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

		// Wait for the end of all the threads
		pool.finish();
	} else {
		QImage normalmap(parameters.input_path);
		if(normalmap.isNull()) {
			status = FAILED;
			error = "Could not load normalmap: " + parameters.input_path;
			return;
		}
		width = normalmap.width();
		height = normalmap.height();
		//convert image into normals

		normals.resize(width * height * 3);
		for(int y = 0; y < height; y++)
			for(int x = 0; x < width; x++) {
				QRgb rgb = normalmap.pixel(x, y);
				int i = 3*(x + y*width);
				normals[i+0] = (qRed(rgb) / 255.0f) * 2.0f - 1.0f;
				normals[i+1] = (qGreen(rgb) / 255.0f) * 2.0f - 1.0f;
				normals[i+2] = (qBlue(rgb) / 255.0f) * 2.0f - 1.0f;
			}
	}

	if(parameters.flatMethod != FLAT_NONE) {
		//TODO: do we really need double precision?
		vector<double> normalsd(normals.size());
		for(uint32_t i = 0; i < normals.size(); i++)
			normalsd[i] = (double)normals[i];

		NormalsImage ni;
		ni.load(normalsd, width, height);
		switch(parameters.flatMethod) {
			case FLAT_NONE: break;
			case FLAT_RADIAL:
				ni.flattenRadial();
				break;
			case FLAT_FOURIER:
				//convert radius to frequencies
				double sigma = 100*parameters.m_FlatRadius;
				ni.flattenFourier(width/10, sigma);
				break;
		}
		normalsd = ni.normals;
		for(uint32_t i = 0; i < normals.size(); i++)
			normals[i] = (float)ni.normals[i];

	}

	if(parameters.compute || parameters.flatMethod != FLAT_NONE) {
		// Save the normals

		vector<uint8_t> normalmap(width * height * 3);
		for(size_t i = 0; i < normals.size(); i++)
			normalmap[i] = floor(((normals[i] + 1.0f) / 2.0f) * 255);

		QImage img(normalmap.data(), width, height, width*3, QImage::Format_RGB888);
	

		// Set spatial resolution if known. Need to convert as pixelSize stored in mm/pixel whereas QImage requires pixels/m
		if( pixelSize > 0 ) {
			int dotsPerMeter = round(1000.0/pixelSize);
			img.setDotsPerMeterX(dotsPerMeter);
			img.setDotsPerMeterY(dotsPerMeter);
		}
		img.save(parameters.path);
	
	}
	

	if(parameters.surface_integration == SURFACE_ASSM) {
		
		bool proceed = progressed("Adaptive mesh normal integration...", 50);
		if(!proceed)
			return;
		QString filename = output.left(output.size() -4) + ".obj";

		assm(filename, normals, parameters.assm_error);

	} else if(parameters.surface_integration == SURFACE_BNI || parameters.surface_integration == SURFACE_FFT) {
		bool proceed = progressed("Bilateral normal integration...", 0);
		if(!proceed)
			return;

		vector<float> z;
		if(parameters.surface_integration == SURFACE_BNI)
			bni_integrate(callback, width, height, normals, z, parameters.bni_k);
		else
			fft_integrate(callback, width, height, normals, z);

		if(z.size() == 0) {
			error = "Failed to integrate normals";
			status = FAILED;
			return;
		}
		//TODO remove extension properly

		progressed("Saving surface...", 99);
		QString filename = output.left(output.size() -4) + ".ply";
		savePly(filename, width, height, z);
	}
	progressed("Done", 100);
}

bool saveObj(const char *filename, pmp::SurfaceMesh &mesh) {
	FILE *fp = fopen(filename, "wb");
	if(!fp) {
//		cerr << "Could not open file: " << filename << endl;
		return false;
	}
	int nvertices = 0;
	for(auto vertex: mesh.vertices()) {
		auto p = mesh.position(vertex);
		fprintf(fp, "v %f %f %f\n", p[0], p[1], p[2]);
		nvertices++;
	}
	for (auto face : mesh.faces()) {
		int indexes[3];
		int count =0 ;
		for (auto vertex : mesh.vertices(face)) {
			auto p = mesh.position(vertex);
			int v = indexes[count++] = vertex.idx() + 1;
			assert(v > 0 && v <= nvertices);
		}
		fprintf(fp, "f %d %d %d\n", indexes[0], indexes[1], indexes[2]);
	}
	fclose(fp);
	return true;
}

void NormalsTask::assm(QString filename, vector<float> &_normals, float approx_error) {
	Grid<Eigen::Vector3f> normals(imageset.width, imageset.height, Eigen::Vector3f(0.0f, 0.0f, 0.0f));
	for(int y = 0; y < imageset.height; y++)
		for(int x = 0; x < imageset.width; x++) {
			int i = 3*(x + y*imageset.width);
			normals.at(y, x) = Eigen::Vector3f(-_normals[i+0], -_normals[i+1], -_normals[i+2]);
		}

	Grid<unsigned char> mask(imageset.width, imageset.height, 0);
	mask.fill(255);

	float l_min = 1;
	float l_max = 100;

	PhotometricRemeshing<pmp::Orthographic> remesher(normals, mask);
	remesher.run(l_min, l_max, approx_error);

	pmp::Integration<double, pmp::Orthographic> integrator(remesher.mesh(), normals, mask);
	integrator.run();

	saveObj(filename.toStdString().c_str(), remesher.mesh());
}


void NormalsWorker::run() {
	switch (solver)
	{
	case NORMALS_L2:
		solveL2();
		break;
	case NORMALS_SBL:
		solveSBL();
		break;
	case NORMALS_RPCA:
		solveRPCA();
		break;
	}

}


void NormalsWorker::solveL2()
{
	vector<Vector3f> &m_Lights = m_Imageset.lights();

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
	//TODO do it in a single large matrix, it should be  faster.
	for (size_t p = 0; p < m_Row.size(); p++) {
		// Fill the pixel vector
		for (size_t m = 0; m < m_Lights.size(); m++)
			mPixel(m, 0) = m_Row[p][m].mean();

		if(m_Imageset.light3d) {
			for(size_t i = 0; i < m_Lights.size(); i++) {
				Vector3f light = m_Imageset.relativeLight(m_Lights[i], p, m_Imageset.height - row);
				light.normalize();
				for (int j = 0; j < 3; j++)
					mLights(i, j) = light[j];
			}
		}

		mNormals = (mLights.transpose() * mLights).ldlt().solve(mLights.transpose() * mPixel);
		mNormals.col(0).normalize();
		assert(!isnan(mNormals.col(0)[0]));
		assert(!isnan(mNormals.col(0)[1]));
		assert(!isnan(mNormals.col(0)[2]));
		m_Normals[normalIdx+0] = float(mNormals.col(0)[0]);
		m_Normals[normalIdx+1] = float(mNormals.col(0)[1]);
		m_Normals[normalIdx+2] = float(mNormals.col(0)[2]);

		normalIdx += 3;
	}
}
void NormalsWorker::solveSBL()
{
}

void NormalsWorker::solveRPCA()
{
}

