#include "brdftask.h"
#include "../src/relight_threadpool.h"
#include "brdf_math.h"

#include <QImage>
#include <QDir>
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
	imageset.setColorProfileMode(COLOR_PROFILE_LINEAR_RGB);
}


void BrdfTask::initFromFolder(const char *folder, Dome &dome, const Crop &folderCrop) {
	imageset.initFromFolder(folder);
	imageset.initFromDome(dome);
	parameters.crop = folderCrop;
	if(folderCrop.width() > 0)
		imageset.setCrop(folderCrop);
	imageset.rotateLights(-folderCrop.angle);
	imageset.setColorProfileMode(COLOR_PROFILE_LINEAR_RGB);
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

	if((parameters.albedo != BrdfParameters::NONE) || parameters.compute_brdf) {
		width = imageset.width;
		height = imageset.height;

		albedo.resize(width * height * 3);
		vector<float> normals, roughness, specular;

		if(parameters.compute_brdf) {
			normals.resize(width * height * 3);
			roughness.resize(width * height);
			specular.resize(width * height * 3);
		}

		RelightThreadPool pool;
		PixelArray line;
		imageset.setCallback(nullptr);
		pool.start(QThread::idealThreadCount());
		for (int i = 0; i < imageset.height; i++) {
			// Read a line
			imageset.readLine(line);

			// Create the normal task and get the run lambda
			uint32_t idx = i * 3 * imageset.width;

			std::function<void(void)> run;

			if (parameters.compute_brdf) {
				float* _a = &albedo[idx];
				float* _n = &normals[idx];
				float* _s = &specular[idx];
				float* _r = &roughness[i * imageset.width];

				BrdfWorker *task = new BrdfWorker(parameters, i, line, _n, _a, _r, _s, imageset, lens);
				run = [task](void)->void {
					task->run();
					delete task;
				};
			} else {
				float* data = &albedo[idx];
				AlbedoWorker *task = new AlbedoWorker(parameters, i, line, data, imageset, lens);
				run = [task](void)->void {
					task->run();
					delete task;
				};
			}

			// Launch the task
			pool.queue(run);
			pool.waitForSpace();

			bool proceed = progressed("Computing...", ((float)i / imageset.height) * 100);
			if(!proceed)
				return;
		}
		pool.finish();

		// Save Maps lambda
		auto saveMap = [&](vector<float>& mapData, QString path, int channels) {
			if (mapData.empty()) return;
			vector<uint8_t> u8_map(width * height * channels);
			for(size_t i = 0; i < mapData.size(); i++) {
				u8_map[i] = std::min(std::max(int(mapData[i]), 0), 255);
			}

			QImage::Format fmt = (channels == 3) ? QImage::Format_RGB888 : QImage::Format_Grayscale8;
			QImage img(u8_map.data(), width, height, width * channels, fmt);

			if(parameters.crop.angle != 0.0f)
				img = parameters.crop.cropBoundingImage(img);

			if( imageset.pixel_size > 0 ) {
				int dotsPerMeter = round(1000.0/imageset.pixel_size);
				img.setDotsPerMeterX(dotsPerMeter);
				img.setDotsPerMeterY(dotsPerMeter);
			}

			bool saved = img.save(destination.filePath(path + ".jpg"), "jpg", parameters.quality);
			if(!saved) {
				error = "Could not save the image: " + destination.filePath(path);
				status = FAILED;
			}
		};

		if (parameters.compute_brdf) {
			saveMap(albedo, parameters.albedo_path, 3);
			saveMap(normals, parameters.normals_path, 3);
			saveMap(roughness, parameters.roughness_path, 1);
			saveMap(specular, parameters.specular_path, 3);
			progressed("BRDF maps done", 100);

			// Generate test renders for visual evaluation
			QString testFolder = destination.filePath("test_renders");
			QDir testDir;
			if (!testDir.exists(testFolder)) {
				testDir.mkpath(testFolder);
			}

			std::vector<Eigen::Vector3f> lights = imageset.lights();
			Eigen::Vector3f V(0.0f, 0.0f, 1.0f); // Orthographic view direction

			for (size_t light_idx = 0; light_idx < lights.size(); ++light_idx) {
				vector<uint8_t> render_data(width * height * 3);

				for (int y = 0; y < height; ++y) {
					for (int x = 0; x < width; ++x) {
						int pixel_idx = (y * width + x);
						int rgb_idx = pixel_idx * 3;
						int normal_idx = rgb_idx;
						int albedo_idx = rgb_idx;
						int rough_idx = pixel_idx;
						int spec_idx = rgb_idx;

						// Unpack data from 8-bit maps (rescale to [0,1])
						Eigen::Vector3f N(
									(normals[normal_idx + 0] / 255.0f - 0.5f) * 2.0f,
								(normals[normal_idx + 1] / 255.0f - 0.5f) * 2.0f,
								(normals[normal_idx + 2] / 255.0f - 0.5f) * 2.0f
								);
						N.normalize();

						Eigen::Vector3f A(
									albedo[albedo_idx + 0] / 255.0f,
								albedo[albedo_idx + 1] / 255.0f,
								albedo[albedo_idx + 2] / 255.0f
								);

						float R = roughness[rough_idx] / 255.0f;

						Eigen::Vector3f S(
									specular[spec_idx + 0] / 255.0f,
								specular[spec_idx + 1] / 255.0f,
								specular[spec_idx + 2] / 255.0f
								);

						// Render using eval_ggx
						// Note: If light_intensity during optimization was 1.0f, use 1.0f here as well.
						// If it was 3.0f, use 3.0f. The intensity needs to match the scale of the observed images.
						// Since we changed it to 1.0f in optimize_brdf_pixel, we changed it here to 1.0f,
						// but if images are dark, it means the *original* observed pixel values were not scaled
						// by PI, therefore the resulting albedo is compensating. Let's make sure it matches.
						// We also shouldn't apply standard sRGB gamma if the optimization was done without it,
						// but usually Qt images are saved/viewed in sRGB, so let's apply simple gamma to the output.
						Eigen::Vector3f color = brdf::eval_ggx(N, lights[light_idx], V, A, R, S, 1.0f);

						// Apply gamma correction (approximate sRGB: color^(1/2.2)) since the BRDF is in linear space
						float gamma = 1.0f / 2.2f;
						color.x() = pow(color.x(), gamma);
						color.y() = pow(color.y(), gamma);
						color.z() = pow(color.z(), gamma);

						// Clamp and convert to 8-bit
						render_data[rgb_idx + 0] = std::min(std::max((uint8_t)(color.x() * 255.0f), (uint8_t)0), (uint8_t)255);
						render_data[rgb_idx + 1] = std::min(std::max((uint8_t)(color.y() * 255.0f), (uint8_t)0), (uint8_t)255);
						render_data[rgb_idx + 2] = std::min(std::max((uint8_t)(color.z() * 255.0f), (uint8_t)0), (uint8_t)255);
					}
				}

				// Save rendered image
				QImage render_img(render_data.data(), width, height, width * 3, QImage::Format_RGB888);
				QString render_path = QDir(testFolder).filePath(QString("render_%1.jpg").arg(light_idx, 3, 10, QChar('0')));
				render_img.save(render_path, "jpg", parameters.quality);
			}

			progressed("Test renders done", 100);
		} else {
			saveMap(albedo, parameters.albedo_path, 3);
			progressed("Albedo done", 100);
		}
	}
	status = DONE;
}

QJsonObject BrdfTask::info() const {
	QJsonObject obj = Task::info();
	obj["parameters"] = parameters.toJson();
	return obj;
}

void AlbedoWorker::run() {
	int nth = (m_Row.nlights-1) * parameters.median_percentage / 100;
	assert(nth >= 0 && nth < m_Row.nlights);
	for(size_t i = 0; i < m_Row.size(); i++) {
		Pixel &p = m_Row[i];
		if(parameters.albedo == BrdfParameters::MEDIAN) {
			//separate components
			std::vector<float> c(p.size());
			for(int k = 0; k < 3; k++) {
				for(size_t j = 0; j < p.size(); j++) {
					c[j] = p[j][k];
				}
				std::nth_element(c.begin(), c.begin() + nth, c.end());
				albedo[i*3 + k] =  c[nth];
			}
		} else if(parameters.albedo == BrdfParameters::MEAN) {
			for(int k = 0; k < 3; k++) {
				albedo[i*3 + k] = 0.0f;
				for(size_t j = 0; j < p.size(); j++) {
					albedo[i*3 + k] += p[j][k];
				}
				albedo[i*3 + k] /= p.size();
			}
		}
	}
}

void BrdfWorker::run() {
	std::vector<Eigen::Vector3f> L = m_Imageset.lights();
	int nth = (m_Row.nlights-1) * parameters.median_percentage / 100;
	if (nth < 0) nth = 0;
	if (nth >= m_Row.nlights) nth = m_Row.nlights - 1;

	for(size_t i = 0; i < m_Row.size(); i++) {
		Pixel p = m_Row[i];
		
		// Compute initial albedo (using Median approach as simplest heuristic)
		Eigen::Vector3f init_albedo_vec(0.1f, 0.1f, 0.1f);
		if(parameters.albedo == BrdfParameters::MEDIAN) {
			std::vector<float> c(p.size());
			for(int k = 0; k < 3; k++) {
				for(size_t j = 0; j < p.size(); j++) c[j] = p[j][k];
				std::nth_element(c.begin(), c.begin() + nth, c.end());
				init_albedo_vec[k] = std::max(c[nth] / 255.0f, 0.001f);
			}
		}

		// Normalize observed pixel colors to [0,1] for mathematical BRDF model
		for(size_t j = 0; j < p.size(); j++) {
			p[j] = p[j] / 255.0f;
		}
		//TODO fit all three components at once instead of computing biological luminance.
		// Initialize Normals via simple Lambertian PS heuristics
		Eigen::VectorXf I_lum(p.size());
		Eigen::MatrixXf L_mat(p.size(), 3);
		for (size_t j = 0; j < p.size(); j++) {
			I_lum(j) = 0.2126f * p[j].r + 0.7152f * p[j].g + 0.0722f * p[j].b;
			L_mat.row(j) = L[j];
		}
		Eigen::Vector3f init_normal = brdf::simple_lambertian_photometric_stereo(I_lum, L_mat);
		init_normal.normalize();
		
		// Handle invalid or negative backward-facing normals
		if (std::isnan(init_normal.x()) || init_normal.z() < 0) {
			init_normal = Eigen::Vector3f(0.0f, 0.0f, 1.0f);
		}

		// Optmize
		brdf::BrdfFitResult result = brdf::optimize_brdf_pixel(
					p, L, init_normal, init_albedo_vec, 0.3f, 1.0f, false, true); // default rough=0.3, light_intensity=1.0
		
		// Output maps with appropriate scaling to standard image arrays (0-255 bounds usually)
		if (normals) {
			normals[i*3 + 0] = std::clamp((result.normal.x() * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f);
			normals[i*3 + 1] = std::clamp((result.normal.y() * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f);
			normals[i*3 + 2] = std::clamp((result.normal.z() * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f);
		}
		if (albedo) {
			albedo[i*3 + 0] = std::clamp(result.albedo.x() * 255.0f, 0.0f, 255.0f);
			albedo[i*3 + 1] = std::clamp(result.albedo.y() * 255.0f, 0.0f, 255.0f);
			albedo[i*3 + 2] = std::clamp(result.albedo.z() * 255.0f, 0.0f, 255.0f);
		}
		if (roughness) {
			// Grey-scale 1-channel output
			roughness[i] = std::clamp(result.roughness * 255.0f, 0.0f, 255.0f);
		}
		if (specular) {
			specular[i*3 + 0] = std::clamp(result.specular.x() * 255.0f, 0.0f, 255.0f);
			specular[i*3 + 1] = std::clamp(result.specular.y() * 255.0f, 0.0f, 255.0f);
			specular[i*3 + 2] = std::clamp(result.specular.z() * 255.0f, 0.0f, 255.0f);
		}
	}
}
