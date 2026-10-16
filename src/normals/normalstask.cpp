#include "normalstask.h"
#include "normalsworker.h"
#include "../jpeg_decoder.h"
#include "../jpeg_encoder.h"
#include "../image_decoder.h"
#include "../imageset.h"
#include "../threadpool.h"
#include "bni_normal_integration.h"
#include "fft_normal_integration.h"
#include "flatnormals.h"
#include "fast_gaussian_blur.h"

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
#include <QPoint>
#include <QTextStream>
#include <vector>
#include <algorithm>
#include <iostream>
#include <time.h>
#include <cmath>
#include <string>

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

void saveNormalmap(	std::vector<Eigen::Vector3f> normals, int width, int height, QString filename) {
	// Save the corrected normals at original (full) resolution.
	vector<uint8_t> normalmap(width * height * 3);
	for(size_t i = 0; i < normals.size(); i++) {
		normalmap[i*3 + 0] = std::min(std::max(round(((normals[i][0] + 1.0f) / 2.0f) * 255.0f), 0.0f), 255.0f);
		normalmap[i*3 + 1] = std::min(std::max(round(((normals[i][1] + 1.0f) / 2.0f) * 255.0f), 0.0f), 255.0f);
		normalmap[i*3 + 2] = std::min(std::max(round(((normals[i][2] + 1.0f) / 2.0f) * 255.0f), 0.0f), 255.0f);
	}
	JpegEncoder enc2;
	enc2.setColorSpace(JCS_RGB, 3);
	enc2.setJpegColorSpace(JCS_RGB);
	enc2.setQuality(100);
	enc2.setOptimize(true);
	enc2.setChromaSubsampling(false);
	//TODO save resolution in exif.
	enc2.encode(normalmap.data(), width, height, filename.toStdString().c_str());
}

static bool saveHighpassHeightmap(const vector<float> &heightmap, int width, int height,
								  double sigma, double low_percentile, int quality, QString filename) {
	vector<float> filtered = heightmap;
	vector<float> blurred = heightmap;
	fast_gaussian_blur(blurred, unsigned(width), unsigned(height), float(sigma));
	for(size_t i = 0; i < filtered.size(); i++)
		filtered[i] -= blurred[i];

	auto minmax = minmax_element(filtered.begin(), filtered.end());
	const float min_value = *minmax.first;
	const float max_value = *minmax.second;
	float low_value = min_value;
	if(low_percentile > 0.0 && max_value > min_value) {
		const size_t bin_count = 1000;
		const double bin_width = (max_value - min_value) / bin_count;
		vector<size_t> histogram(bin_count, 0);
		for(float value: filtered) {
			size_t bin = min(size_t((value - min_value) / bin_width), bin_count - 1);
			histogram[bin]++;
		}
		const size_t target_count = size_t(ceil(low_percentile * filtered.size() / 100.0));
		size_t cumulative_count = 0;
		for(size_t bin = 0; bin < bin_count; bin++) {
			cumulative_count += histogram[bin];
			if(cumulative_count >= max(size_t(1), target_count)) {
				const size_t preceding_count = cumulative_count - histogram[bin];
				const double fraction =
					double(max(size_t(1), target_count) - preceding_count) / histogram[bin];
				low_value = float(min_value + (bin + fraction) * bin_width);
				break;
			}
		}
	}

	const float high_value = min_value + 0.85f * (max_value - min_value);
	const float contrast_range = high_value - low_value;
	QImage highpass(width, height, QImage::Format_Grayscale8);
	for(int y = 0; y < height; y++) {
		uchar *row = highpass.scanLine(y);
		for(int x = 0; x < width; x++) {
			const float value = filtered[size_t(x) + size_t(y) * size_t(width)];
			const float mapped = contrast_range > 0.0f
				? std::clamp(2.0f * (value - low_value) / contrast_range, 0.0f, 1.0f)
				: 0.0f;
			row[x] = uchar(lround(mapped * 255.0f));
		}
	}

	return highpass.save(filename, "JPG", quality);
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

	if(parameters.heightmap_input) {
		ImageDecoder decoder;
		const std::string input_path = parameters.input_path.toStdString();
		int width = 0;
		int height = 0;
		if(!decoder.init(input_path.c_str(), width, height)) {
			error = "Could not load heightmap: " + parameters.input_path;
			status = FAILED;
			return;
		}
		if(decoder.numChannels() != 1) {
			decoder.finish();
			error = "Heightmap TIFF must have one channel: " + parameters.input_path;
			status = FAILED;
			return;
		}

		vector<float> z(size_t(width) * size_t(height));
		const size_t rows_read = decoder.readRows(height, z.data());
		const bool decoder_finished = decoder.finish();
		if(rows_read != size_t(height) || !decoder_finished) {
			error = "Failed to read heightmap: " + parameters.input_path;
			status = FAILED;
			return;
		}

		QString filename = destination.filePath(QString("heightmap_highpass_sigma_%1_low_%2.jpg")
			.arg(parameters.filter_sigma, 0, 'g', 6)
			.arg(parameters.low_percentile, 0, 'g', 6));
		if(!saveHighpassHeightmap(z, width, height, parameters.filter_sigma,
								  parameters.low_percentile, parameters.quality, filename)) {
			error = "Failed to save high-pass map to: " + filename;
			status = FAILED;
			return;
		}

		progressed("Done", 100);
		status = DONE;
		return;
	}

	function<bool(QString s, int d)> callback = [this](QString s, int n)->bool { return this->progressed(s, n); };

	std::vector<Eigen::Vector3f> normals;

	int width = 0, height = 0;

	if(parameters.compute) {
		mime = IMAGE;
		width = imageset.width;
		height = imageset.height;

		normals.resize(width * height);
		ThreadPool pool;
		PixelArray line;
		imageset.setCallback(nullptr);
		pool.start(QThread::idealThreadCount());

		for (int i = 0; i < height; i++) {
			// Read a line
			imageset.readLine(line);

			// Create the normal task and get the run lambda
			uint32_t idx = i * width;
			Eigen::Vector3f* data = &normals[idx];

			NormalsWorker *task = new NormalsWorker(parameters.solver, i, line, data, imageset,
				parameters.robust_threshold_high, parameters.robust_threshold_low);

			std::function<void(void)> run = [task](void)->void {
				task->run();
				delete task;
			};

			// Launch the task
			pool.queue(run);
			pool.waitForSpace();

			bool proceed = progressed("Computing normals...", ((float)i / imageset.height) * 100);
			if(!proceed) {
				pool.abort();
				status = FAILED;
				return;
			}
		}

		// Wait for the end of all the threads
		pool.finish();
		if(parameters.crop.angle != 0.0f) {
			//rotate and crop the normals.
			normals = parameters.crop.cropBoundingNormals(normals, width, height);
		}
		//check no normals with z == 0.
		for(Eigen::Vector3f &n: normals) {
			fixNormal(n);
		}

	} else {
		const QString suffix = QFileInfo(parameters.input_path).suffix().toLower();
		if(suffix == "tif" || suffix == "tiff") {
			ImageDecoder decoder;
			const std::string input_path = parameters.input_path.toStdString();
			if(!decoder.init(input_path.c_str(), width, height)) {
				status = FAILED;
				error = "Could not load normalmap: " + parameters.input_path;
				return;
			}
			const int channels = decoder.numChannels();
			if(channels != 1 && channels != 3 && channels != 4) {
				decoder.finish();
				status = FAILED;
				error = "Unsupported normalmap channel count: " + parameters.input_path;
				return;
			}
			vector<float> pixels(size_t(width) * size_t(height) * size_t(channels));
			const size_t rows_read = decoder.readRows(height, pixels.data());
			const bool decoder_finished = decoder.finish();
			if(rows_read != size_t(height) || !decoder_finished) {
				status = FAILED;
				error = "Failed to read normalmap: " + parameters.input_path;
				return;
			}
			normals.resize(size_t(width) * size_t(height));
			for(size_t i = 0; i < normals.size(); i++) {
				const float *pixel = pixels.data() + i * size_t(channels);
				normals[i][0] = 2.0f * pixel[0] - 1.0f;
				normals[i][1] = 2.0f * pixel[channels > 1 ? 1 : 0] - 1.0f;
				normals[i][2] = 2.0f * pixel[channels > 2 ? 2 : 0] - 1.0f;
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
			normals.resize(size_t(width) * size_t(height));
			for(int y = 0; y < height; y++) {
				for(int x = 0; x < width; x++) {
					QRgb rgb = normalmap.pixel(x, y);
					int i = x + y*width;
					normals[size_t(i)][0] = (qRed(rgb) / 255.0f) * 2.0f - 1.0f;
					normals[size_t(i)][1] = (qGreen(rgb) / 255.0f) * 2.0f - 1.0f;
					normals[size_t(i)][2] = (qBlue(rgb) / 255.0f) * 2.0f - 1.0f;
				}
			}
		}
	}

	float downsampling = 1.0;

	const bool will_integrate_plane =
		parameters.flatMethod == FLAT_PLANE &&
		(parameters.surface_integration == SURFACE_BNI ||
		 parameters.surface_integration == SURFACE_FFT);


	if(parameters.flatMethod != FLAT_NONE) {

		switch(parameters.flatMethod) {
			case FLAT_NONE: break;
			case FLAT_RADIAL:
				flattenRadialNormals(width, height, normals, 100);
				break;
			case FLAT_PLANE:
			{
				if(!will_integrate_plane)
					flattenPlaneNormals(width, height, normals, parameters.plane_points,
						parameters.crop, imageset.image_width, imageset.image_height);
				break;
			}
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


	QString normalmap_path = destination.filePath(parameters.normalsname + ".jpg");

	//Save these values for later, if using flat plane, which needs to integrate the surface to fix the normalmap.

	const int normals_width  = width;
	const int normals_height = height;
	std::vector<Eigen::Vector3f> full_normals;

	if(parameters.flatMethod != FLAT_PLANE)
		saveNormalmap(normals, width, height, normalmap_path);

	if(parameters.surface_width != 0 &&
		(parameters.surface_width != width || parameters.surface_height != height)) {
		//scale normals, use full_normals temporarily
		full_normals.resize(parameters.surface_width*parameters.surface_height);
		bilinear_interpolation3f((Eigen::Vector3f *)normals.data(), width, height,
					   parameters.surface_width, parameters.surface_height, (Eigen::Vector3f *)full_normals.data());
		swap(full_normals, normals);

		for(Eigen::Vector3f &n: normals) {
			fixNormal(n);
		}

		downsampling = float(width)/parameters.surface_width;
		width = parameters.surface_width;
		height = parameters.surface_height;
	}

	if(parameters.surface_integration == SURFACE_ASSM) {
		
		bool proceed = progressed("Adaptive mesh normal integration...", 50);
		if(!proceed) { status = FAILED; return; }
		//TODO move to saveply
		QString filename = destination.filePath("3D_surface.ply");

		assm(filename, normals, width, height, downsampling, &callback);

	} else if(parameters.surface_integration == SURFACE_BNI || parameters.surface_integration == SURFACE_FFT) {
		QString type = parameters.surface_integration == SURFACE_BNI ? "Bilateral" : "Fourier";
		bool proceed = progressed(type + " normal integration...", 0);
		if(!proceed) { status = FAILED; return; }

		vector<float> z;
		if(parameters.surface_integration == SURFACE_BNI) {
			try {
				bool proceed = bni_integrate(callback, width, height, normals, z, parameters.bni_k);
				if(!proceed) { status = FAILED; return; }
			} catch(std::string err) {
				error = err.c_str();
				status = FAILED;
				return;
			}

		} else {
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

		if(parameters.flatMethod == FLAT_RADIAL) {
			flattenRadialHeights(width, height, z);
		}
		// 4-point plane height flattening.
		if(parameters.flatMethod == FLAT_PLANE) {
			bool proceed = progressed("Flattening surface (plane)...", 0);
			if(!proceed) { status = FAILED; return; }
			double pa = 0, pb = 0, pd = 0;
			SurfaceCoeffs sc;
			flattenPlaneHeights(width, height, z, parameters.plane_points,
				parameters.crop, imageset.image_width, imageset.image_height,
				nullptr, &sc);

			// Apply the same correction to the full-res normals.
			// rescaled() adjusts 4th-order coefficients by 1/s⁴ and 2nd-order by 1/s².
			double s = (double)normals_width/width;
			applyNormalCorrection(normals_width, normals_height, full_normals, sc.rescaled(s));
		}

		progressed("Saving surface...", 99);
		QString filename = destination.filePath("3D_surface.ply");
		float pixel_size = imageset.pixel_size * downsampling;
		if(!savePly(filename, width, height, z, downsampling, pixel_size)) {
			error = "Failed to save .ply to: " + filename;
			status = FAILED;
			return;
		}
		invertZ(z);
		filename = destination.filePath("heightmap.tiff");
		if(!saveTiff(filename, width, height, z, false, pixel_size)) {
			error = "Failed to save depth map to: " + filename;
			status = FAILED;
			return;
		}
		filename = destination.filePath("heightmap_normalized.tiff");
		if(!saveTiff(filename, width, height, z, true, pixel_size)) {
			error = "Failed to save depth map to: " + filename;
			status = FAILED;
			return;
		}

		if(parameters.filter_sigma > 0.0) {
			filename = destination.filePath(QString("heightmap_highpass_sigma_%1_low_%2.jpg")
				.arg(parameters.filter_sigma, 0, 'g', 6)
				.arg(parameters.low_percentile, 0, 'g', 6));
			if(!saveHighpassHeightmap(z, width, height, parameters.filter_sigma,
									  parameters.low_percentile, parameters.quality, filename)) {
				error = "Failed to save high-pass map to: " + filename;
				status = FAILED;
				return;
			}
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
		stream << "property list uchar int vertex_indices\n";
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

void NormalsTask::assm(QString filename, std::vector<Eigen::Vector3f> &_normals, int width, int height, float downsampling,
					   std::function<bool(QString stage, int percent)> *callback) {
	float approx_error = parameters.assm_error;
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
	float pixel_size = imageset.pixel_size * downsampling;
	savePly(filename.toStdString().c_str(), mesh, width, height, pixel_size);
}
