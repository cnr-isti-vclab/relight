#include "normalstask.h"
#include "normalsworker.h"
#include "../jpeg_decoder.h"
#include "../jpeg_encoder.h"
#include "../imageset.h"
#include "../relight_threadpool.h"
#include "bni_normal_integration.h"
#include "fft_normal_integration.h"
#include "flatnormals.h"

#include <QThread>

#include <assm/Grid.h>
#include <assm/algorithms/PhotometricRemeshing.h>
#include <assm/algorithms/Integration.h>

#include <Eigen/Eigen>
#include <Eigen/Core>
#include <Eigen/Dense>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QImage>
#include <QTextStream>
#include <vector>
#include <algorithm>
#include <iostream>
#include <time.h>
#include <cmath>

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
	//imageset.width = imageset.image_width = project.lens.width;
	//imageset.height = imageset.image_height = project.lens.height;
	//img_size = project.imgsize;

	imageset.initFromProject(project);

	parameters.crop = project.crop;
	imageset.setCrop(parameters.crop, project.offsets);
	imageset.rotateLights(-parameters.crop.angle);

	imageset.pixel_size = project.pixelSize();
}

void NormalsTask::initFromFolder(const char *folder, Dome &dome, const Crop &folderCrop) {
	imageset.initFromFolder(folder);
	imageset.initFromDome(dome);
	parameters.crop = folderCrop;
	if(folderCrop.width() > 0)
		imageset.setCrop(folderCrop);
	imageset.rotateLights(-folderCrop.angle);
}

void NormalsTask::setParameters(NormalsParameters &param) {
	parameters = param;
	label = parameters.summary();
}

void invertZ(vector<float> &z) {
	float min = 1e20;
	float max = -1e20;
	for(float v: z) {
		min = std::min(min, v);
		max = std::max(max, v);
	}
	for(float &v: z) {
		v = max - (v - min);
	}
}


void NormalsTask::run() {
	status = RUNNING;
	startedAt = QDateTime::currentDateTimeUtc();
	
	label = parameters.summary();

	QDir destination(parameters.path);
	if(!destination.exists()) {
		if(!QDir().mkpath(parameters.path)) {
			error = "Could not create normals folder: " + parameters.path;
			status = FAILED;
			return;
		}
	}


	function<bool(QString s, int d)> callback = [this](QString s, int n)->bool { return this->progressed(s, n); };

	std::vector<Eigen::Vector3f> normals;

	int width = 0, height = 0;

	if(parameters.compute) {
		mime = IMAGE;
		width = imageset.width;
		height = imageset.height;

	normals.resize(width * height);
		RelightThreadPool pool;
		PixelArray line;
		imageset.setCallback(nullptr);
		pool.start(parameters.debug_shadows ? 1 : QThread::idealThreadCount());

		const int nLights = (int)imageset.lights().size();
		if (parameters.debug_shadows)
			initWeightsDebug(destination, width, height);

		for (int i = 0; i < height; i++) {
			// Read a line
			imageset.readLine(line);

			// Create the normal task and get the run lambda
			uint32_t idx = i * width;
			Eigen::Vector3f* data = &normals[idx];

			NormalsWorker *task = new NormalsWorker(parameters.solver, i, line, data, imageset,
				parameters.robust_threshold_high, parameters.robust_threshold_low);

			if (parameters.debug_shadows) {
				const int nP = std::min((int)line.size(), width);
				auto wbuf = std::make_shared<std::vector<float>>(width * nLights, 0.0f);
				auto lineCopy = std::make_shared<PixelArray>(line);
				task->setWeightsOutput(wbuf->data());
				std::function<void(void)> run = [task, this, wbuf, lineCopy, nP](void)->void {
					task->run();
					delete task;
					writeWeightsDebugRow(wbuf->data(), *lineCopy, nP);
				};
				pool.queue(run);
			} else {
				std::function<void(void)> run = [task](void)->void {
					task->run();
					delete task;
				};
				pool.queue(run);
			}
			pool.waitForSpace();

			bool proceed = progressed("Computing normals...", ((float)i / imageset.height) * 100);
			if(!proceed)
				return;
		}

		// Wait for the end of all the threads
		pool.finish();
		if (parameters.debug_shadows)
			finishWeightsDebug();
		if(parameters.crop.angle != 0.0f) {
			//rotate and crop the normals.
			normals = parameters.crop.cropBoundingNormals(normals, width, height);
		}
		//check no normals with z == 0.
		for(Eigen::Vector3f &n: normals) {
			fixNormal(n);
		}

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

		normals.resize(width * height);
		for(int y = 0; y < height; y++) {
			for(int x = 0; x < width; x++) {
				QRgb rgb = normalmap.pixel(x, y);
				int i = (x + y*width);
				normals[i][0] = (qRed(rgb) / 255.0f) * 2.0f - 1.0f;
				normals[i][1] = (qGreen(rgb) / 255.0f) * 2.0f - 1.0f;
				normals[i][2] = (qBlue(rgb) / 255.0f) * 2.0f - 1.0f;
			}
		}
	}

	if(parameters.flatMethod != FLAT_NONE) {

		switch(parameters.flatMethod) {
			case FLAT_NONE: break;
			case FLAT_RADIAL:
				flattenRadialNormals(width, height, normals, 100);
				break;
			case FLAT_FOURIER:
				try {
					//convert radius to frequencies
					double sigma = width*parameters.fourierPercentage/100.0;
					flattenFourierNormals(width, height, normals, 0.2, sigma);
				} catch(std::length_error e) {
					return;
				}
				break;
			case FLAT_BLUR:
				double sigma = width*parameters.blurPercentage/100.0;
				flattenBlurNormals(width, height, normals, sigma);
			break;

		}
	}

	float downsampling = 1.0;

	if(parameters.compute) {
		// Save the normals

		vector<uint8_t> normalmap(width * height * 3);
		for(size_t i = 0; i < normals.size(); i++) {
			normalmap[i*3 + 0] = std::min(std::max(round(((normals[i][0] + 1.0f) / 2.0f) * 255.0f), 0.0f), 255.0f);
			normalmap[i*3 + 1] = std::min(std::max(round(((normals[i][1] + 1.0f) / 2.0f) * 255.0f), 0.0f), 255.0f);
			normalmap[i*3 + 2] = std::min(std::max(round(((normals[i][2] + 1.0f) / 2.0f) * 255.0f), 0.0f), 255.0f);
		}

		// Normal maps encode XYZ direction vectors (remapped to [0,255]).
		// This is geometry data, NOT color: do NOT apply any color transform.
		JpegEncoder encoder;
		encoder.setColorSpace(JCS_RGB, 3);
		encoder.setJpegColorSpace(JCS_RGB);
		encoder.setQuality(100);
		encoder.setOptimize(true);
		encoder.setChromaSubsampling(false); // No chroma subsampling for normal maps
		
		// Set spatial resolution if known
		/*if(imageset.pixel_size > 0) {
			float dotsPerMeter = 1000.0f / imageset.pixel_size;
			encoder.setDotsPerMeter(dotsPerMeter);
		}*/
		//TODO png could be another format!
		QString filename = destination.filePath(parameters.normalsname + ".jpg");
		if(!encoder.encode(normalmap.data(), width, height, filename.toStdString().c_str())) {
			error = "Failed to save normal map JPEG: " + filename;
			status = FAILED;
			return;
		}
	}

	if(parameters.surface_width != 0 &&
		(parameters.surface_width != width || parameters.surface_height != height)) {
		//scale normals.
		std::vector<Eigen::Vector3f> tmp(parameters.surface_width*parameters.surface_height);

		bilinear_interpolation3f((Eigen::Vector3f *)normals.data(), width, height,
					   parameters.surface_width, parameters.surface_height, (Eigen::Vector3f *)tmp.data());
		swap(tmp, normals);

		for(Eigen::Vector3f &n: normals) {
			fixNormal(n);
		}

		width = parameters.surface_width;
		height = parameters.surface_height;
		downsampling = float(width)/parameters.surface_width;
	}

	if(parameters.surface_integration == SURFACE_ASSM) {
		
		bool proceed = progressed("Adaptive mesh normal integration...", 50);
		if(!proceed)
			return;
		//TODO move to saveply
		QString filename = destination.filePath("3D_surface.ply");

		assm(filename, normals, width, height, parameters.assm_error, &callback);

	} else if(parameters.surface_integration == SURFACE_BNI || parameters.surface_integration == SURFACE_FFT) {
		QString type = parameters.surface_integration == SURFACE_BNI ? "Bilateral" : "Fourier";
		bool proceed = progressed(type + " normal integration...", 0);
		if(!proceed)
			return;

		vector<float> z;
		if(parameters.surface_integration == SURFACE_BNI)
			bni_integrate(callback, width, height, normals, z, parameters.bni_k);
		else {

			try {
				fft_integrate(callback, width, height, normals, z);
			} catch(std::length_error e) {
				error = "Failed to integrate normals, length error.";
				status = FAILED;
				return;
			}
		}

		if(z.size() == 0) {
			error = "Failed to integrate normals";
			status = FAILED;
			return;
		}
		//TODO remove extension properly

		progressed("Saving surface...", 99);
		QString filename = destination.filePath("3D_surface.ply");
		if(!savePly(filename, width, height, z, downsampling, imageset.pixel_size)) {
			error = "Failed to save .ply to: " + filename;
			status = FAILED;
			return;
		}
		invertZ(z);
		filename = destination.filePath("heightmap.tiff");
		if(!saveTiff(filename, width, height, z, false, imageset.pixel_size)) {
			error = "Failed to save depth map to: " + filename;
			status = FAILED;
			return;
		}
		filename = destination.filePath("heightmap_normalized.tiff");
		if(!saveTiff(filename, width, height, z, true, imageset.pixel_size)) {
			error = "Failed to save depth map to: " + filename;
			status = FAILED;
			return;
		}
	}
	progressed("Done", 100);
	status = DONE;
}

void NormalsTask::fixNormal(Eigen::Vector3f &n) {
	if(std::isnan(n[0]) || std::isnan(n[1]) || std::isnan(n[2]) || n[2] < z_threshold) {
		if(std::isnan(n[0])) n[0] = 0.0;
		if(std::isnan(n[1])) n[1] = 0.0;
		if(std::isnan(n[2]) || n[2] < z_threshold)
			n[2] = z_threshold;
		n.normalize();
	}
}

QJsonObject NormalsTask::info() const {
	QJsonObject obj = Task::info();
	obj["parameters"] = parameters.toJson();
	return obj;
}

bool savePly(const char *filename, pmp::SurfaceMesh &mesh, int width, int height, float pixel_size) {
	QFile file(filename);
	bool success = file.open(QFile::WriteOnly);
	if(!success)
		return false;

	float scale = (pixel_size > 0) ? pixel_size : 1.0f;
	float cx = (width  - 1) * 0.5f;
	float cy = (height - 1) * 0.5f;

	std::vector<float> vertices;
	for(auto vertex: mesh.vertices()) {
		auto p = mesh.position(vertex);
		float u = p[0] / (width  - 1);
		float v = p[1] / (height - 1);
		vertices.push_back((p[0] - cx) * scale);
		vertices.push_back((p[1] - cy) * scale);
		vertices.push_back(p[2] * scale);
		vertices.push_back(u);
		vertices.push_back(v);
	}
	std::vector<uint8_t> indices;

	int count =0 ;
	for (auto face : mesh.faces()) {
		indices.resize(13*(count+1));
		uint8_t *start = &indices[13*count];
		start[0] = 3;
		int *triangle = (int *)(start + 1);

		int i = 0;
		int index[3];
		for (auto vertex : mesh.vertices(face)) {
			index[i++] = vertex.idx();

		}
		//flip triangle order for consistency.
		triangle[0] = index[0];
		triangle[1] = index[2];
		triangle[2] = index[1];
		count++;
	}

	{
		QTextStream stream(&file);

		stream << "ply\n";
		stream << "format binary_little_endian 1.0\n";
		stream << "element vertex " << vertices.size()/5 << "\n";
		stream << "property float x\n";
		stream << "property float y\n";
		stream << "property float z\n";
		stream << "property float s\n";
		stream << "property float t\n";
		stream << "element face " << indices.size()/13 << "\n";
		stream << "property list uchar int vertex_index\n";
		stream << "end_header\n";
	}


	file.write((const char *)vertices.data(), vertices.size()*4);
	file.write((const char *)indices.data(), indices.size());
	file.close();
	return true;
}

bool saveObj(const char *filename, pmp::SurfaceMesh &mesh, int width, int height, float pixel_size) {
	FILE *fp = fopen(filename, "wb");
	if(!fp) {
//		cerr << "Could not open file: " << filename << endl;
		return false;
	}
	float scale = (pixel_size > 0) ? pixel_size : 1.0f;
	float cx = (width  - 1) * 0.5f;
	float cy = (height - 1) * 0.5f;

	int nvertices = 0;
	for(auto vertex: mesh.vertices()) {
		auto p = mesh.position(vertex);
		fprintf(fp, "v %f %f %f\n",
			(p[0] - cx) * scale,
			(p[1] - cy) * scale,
			p[2] * scale);
		nvertices++;
	}
	for(auto vertex: mesh.vertices()) {
		auto p = mesh.position(vertex);
		fprintf(fp, "vt %f %f\n",
			p[0] / (width  - 1),
			p[1] / (height - 1));
	}
	for (auto face : mesh.faces()) {
		int indexes[3];
		int count = 0;
		for (auto vertex : mesh.vertices(face)) {
			int v = indexes[count++] = vertex.idx() + 1;
			assert(v > 0 && v <= nvertices);
		}
		fprintf(fp, "f %d/%d %d/%d %d/%d\n",
			indexes[0], indexes[0],
			indexes[1], indexes[1],
			indexes[2], indexes[2]);
	}
	fclose(fp);
	return true;
}

void NormalsTask::assm(QString filename, std::vector<Eigen::Vector3f> &_normals, int width, int height, float approx_error,
					   std::function<bool(QString stage, int percent)> *callback) {
	Grid<Eigen::Vector3f> normals(width, height, Eigen::Vector3f(0.0f, 0.0f, 0.0f));
	for(int y = 0; y < height; y++)
		for(int x = 0; x < width; x++) {
			int i = (x + y*width);
			normals.at(y, x) = Eigen::Vector3f(-_normals[i][0], -_normals[i][1], -_normals[i][2]);
		}

	Grid<unsigned char> mask(width, height, 0);
	mask.fill(255);

	float l_min = 1;
	float l_max = 100;

	if(callback && !(*callback)("Remeshing...", 0))
		return;
	PhotometricRemeshing<pmp::Orthographic> remesher(normals, mask);
	remesher.run(l_min, l_max, approx_error, 10, true, callback);

	pmp::Integration<double, pmp::Orthographic> integrator(remesher.mesh(), normals, mask);
	integrator.run(callback);
	//flip y and z.
	auto &mesh = remesher.mesh();

	for(auto vertex: mesh.vertices()) {
		auto &p = mesh.position(vertex);
		p[1] = height -p[1] -1;
		p[2] *= -1;
	}
	savePly(filename.toStdString().c_str(), mesh, width, height, imageset.pixel_size);
}

// ---- Weight debug output ------------------------------------------------

void NormalsTask::initWeightsDebug(QDir &destination, int width, int height)
{
	m_wdNLights = (int)imageset.lights().size();
	m_wdWidth   = width;
	imageset.createOutputColorTransform(COLOR_PROFILE_SRGB);
	const int outW = width * 2;
	m_wdEncoders.resize(m_wdNLights);
	for (int m = 0; m < m_wdNLights; m++) {
		m_wdEncoders[m] = new JpegEncoder;
		m_wdEncoders[m]->setColorSpace(JCS_RGB, 3);
		m_wdEncoders[m]->setJpegColorSpace(JCS_RGB);
		m_wdEncoders[m]->setQuality(95);
		m_wdEncoders[m]->setChromaSubsampling(false);
		QString filename = destination.filePath(
		    QString("weight_%1.jpg").arg(m, 3, 10, QChar('0')));
		m_wdEncoders[m]->init(filename.toStdString().c_str(), outW, height);
	}
	m_wdMeanEncoder = new JpegEncoder;
	m_wdMeanEncoder->setColorSpace(JCS_RGB, 3);
	m_wdMeanEncoder->setJpegColorSpace(JCS_RGB);
	m_wdMeanEncoder->setQuality(95);
	m_wdMeanEncoder->setChromaSubsampling(false);
	m_wdMeanEncoder->init(
	    destination.filePath("weight_mean.jpg").toStdString().c_str(), outW, height);
}

void NormalsTask::writeWeightsDebugRow(const float *weights, const PixelArray &line, int nP)
{
	// rowBuf: left half = original image (sRGB), right half = weight (grayscale mapped to RGB).
	std::vector<uint8_t> rowBuf(m_wdWidth * 2 * 3);
	std::vector<float>   linRow(nP * 3);

	// Per-light weight images.
	for (int m = 0; m < m_wdNLights; m++) {
		// Left panel: original image for this light → sRGB.
		for (int x = 0; x < nP; x++) {
			const Color3f &c = line[x][m];
			linRow[x*3 + 0] = c.r / 255.0f;
			linRow[x*3 + 1] = c.g / 255.0f;
			linRow[x*3 + 2] = c.b / 255.0f;
		}
		imageset.applyOutputColorTransformFloat(linRow.data(), rowBuf.data(), nP);

		// Right panel: solver weight → grayscale RGB.
		for (int x = 0; x < nP; x++) {
			float w = weights[x * m_wdNLights + m];
			uint8_t g = (uint8_t)std::min(255, std::max(0, (int)(w * 255.0f)));
			rowBuf[(m_wdWidth + x)*3 + 0] = g;
			rowBuf[(m_wdWidth + x)*3 + 1] = g;
			rowBuf[(m_wdWidth + x)*3 + 2] = g;
		}
		m_wdEncoders[m]->writeRows(rowBuf.data(), 1);
	}

	// Mean weight image: same left panel using light 0 as representative original.
	for (int x = 0; x < nP; x++) {
		const Color3f &c = line[x][0];
		linRow[x*3 + 0] = c.r / 255.0f;
		linRow[x*3 + 1] = c.g / 255.0f;
		linRow[x*3 + 2] = c.b / 255.0f;
	}
	imageset.applyOutputColorTransformFloat(linRow.data(), rowBuf.data(), nP);
	for (int x = 0; x < nP; x++) {
		float sum = 0.0f;
		for (int m = 0; m < m_wdNLights; m++)
			sum += weights[x * m_wdNLights + m];
		uint8_t g = (uint8_t)std::min(255, std::max(0, (int)(sum / m_wdNLights * 255.0f)));
		rowBuf[(m_wdWidth + x)*3 + 0] = g;
		rowBuf[(m_wdWidth + x)*3 + 1] = g;
		rowBuf[(m_wdWidth + x)*3 + 2] = g;
	}
	m_wdMeanEncoder->writeRows(rowBuf.data(), 1);
}

void NormalsTask::finishWeightsDebug()
{
	for (auto *enc : m_wdEncoders) {
		enc->finish();
		delete enc;
	}
	m_wdMeanEncoder->finish();
	delete m_wdMeanEncoder;
	m_wdMeanEncoder = nullptr;
	m_wdEncoders.clear();
	m_wdNLights = 0;
}

// ---- Shadow debug output ------------------------------------------------
//
// For each light saves a side-by-side PNG:
//   left  half = original cropped image (colour)
//   right half = shadow mask (black = Lambertian-consistent, white = shadow/highlight)
//
// The mask is computed with computeShadowMask(normal=(0,0,1)) so it works even
// before normals are known.  Use it to visually verify the detector.

void NormalsTask::saveShadowDebug(QDir &destination, int width, int height)
{
	std::vector<Eigen::Vector3f> lights = imageset.lights();
	for (Eigen::Vector3f &l : lights)
		l.normalize();
	const int nL = (int)lights.size();
	if (nL == 0 || width == 0 || height == 0) return;

	progressed("Computing shadow masks...", 0);

	// Set up linear→sRGB output transform so the left panel looks correct.
	imageset.createOutputColorTransform(COLOR_PROFILE_SRGB);

	// Build the KNN graph once; reuse it for every pixel in every row.
	const LightKNN knn = NormalsWorker::buildLightKNN(lights);

	// One encoder per light. Output image is width*2 wide (left=original, right=mask).
	const int outW = width * 2;
	std::vector<JpegEncoder> encoders(nL);
	for (int m = 0; m < nL; m++) {
		encoders[m].setColorSpace(JCS_RGB, 3);
		encoders[m].setJpegColorSpace(JCS_RGB);
		encoders[m].setQuality(95);
		encoders[m].setChromaSubsampling(false);
		QString filename = destination.filePath(
		    QString("shadow_debug_%1.jpg").arg(m, 3, 10, QChar('0')));
		encoders[m].init(filename.toStdString().c_str(), outW, height);
	}

	// Extra encoder for the per-pixel shadow-percentage image (mean prob over all lights).
	JpegEncoder shadowPctEncoder;
	{
		shadowPctEncoder.setColorSpace(JCS_GRAYSCALE, 1);
		shadowPctEncoder.setJpegColorSpace(JCS_GRAYSCALE);
		shadowPctEncoder.setQuality(95);
		shadowPctEncoder.setChromaSubsampling(false);
		QString filename = destination.filePath("shadow_percentage.jpg");
		shadowPctEncoder.init(filename.toStdString().c_str(), width, height);
	}

	// Per-light row buffer: 3 bytes per pixel × outW pixels.
	std::vector<uint8_t> rowBuf(outW * 3);
	// Grayscale row buffer for the shadow-percentage image.
	std::vector<uint8_t> pctRowBuf(width);
	// Float buffer for one row of one light, normalised to [0, 1].
	std::vector<float> linRow(width * 3);

	// Default normal used before any per-pixel normal is known.
	const Eigen::Vector3f defaultNormal(0.0f, 0.0f, 1.0f);

	imageset.restart();
	PixelArray line;
	for (int y = 0; y < height; y++) {
		imageset.readLine(line);

		// computeShadowMask never uses m_Normals so nullptr is safe.
		NormalsWorker worker(parameters.solver, y, line, nullptr, imageset,
		                     parameters.robust_threshold_high, parameters.robust_threshold_low);

		int nP = std::min((int)line.size(), width);

		// Collect per-pixel per-light shadow probabilities for this row.
		// pixelMasks[x](m) = shadow probability for pixel x, light m.
		std::vector<Eigen::VectorXf> pixelMasks(nP);
		for (int x = 0; x < nP; x++)
			pixelMasks[x] = worker.computeShadowMask(x, defaultNormal, lights, knn);

		// Shadow-percentage row: mean shadow probability across all lights.
		for (int x = 0; x < nP; x++) {
			float mean = pixelMasks[x].sum() / float(nL);
			pctRowBuf[x] = (uint8_t)std::min(255, std::max(0, (int)(mean * 255.0f)));
		}
		shadowPctEncoder.writeRows(pctRowBuf.data(), 1);

		for (int m = 0; m < nL; m++) {
			// Build normalised float row for this light and convert to sRGB uint8.
			for (int x = 0; x < nP; x++) {
				const Color3f &c = line[x][m];
				linRow[x*3 + 0] = c.r / 255.0f;
				linRow[x*3 + 1] = c.g / 255.0f;
				linRow[x*3 + 2] = c.b / 255.0f;
			}
			// applyOutputColorTransformFloat writes sRGB uint8 into the left panel.
			imageset.applyOutputColorTransformFloat(linRow.data(), rowBuf.data(), nP);

			// Right panel – shadow probability → grayscale.
			for (int x = 0; x < nP; x++) {
				float prob = pixelMasks[x](m);
				uint8_t g = (uint8_t)std::min(255, std::max(0, (int)(prob * 255.0f)));
				rowBuf[(width + x)*3 + 0] = g;
				rowBuf[(width + x)*3 + 1] = g;
				rowBuf[(width + x)*3 + 2] = g;
			}
			encoders[m].writeRows(rowBuf.data(), 1);
		}

		progressed("Computing shadow masks...", (y * 100) / height);
	}

	for (int m = 0; m < nL; m++)
		encoders[m].finish();
	shadowPctEncoder.finish();

	progressed("Shadow masks saved", 100);
}




