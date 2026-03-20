#include "brdftask.h"
#include "../src/relight_threadpool.h"
#include "../src/icc_profiles.h"
#include "brdf_math.h"

#include <QImage>
#include <QDir>
#include <QMutexLocker>
#include <QPainter>
#include <QPen>
#include <cmath>
#include <vector>
#include <sstream>
#include <lcms2.h>
using namespace std;



// Draws a reflectance-vs-angle plot for a single pixel:
//   - grey dots:  measured luminance at each light angle (angle between L and V)
//   - dark curve: fitted BRDF evaluated by sweeping L in the plane of N at the same angles
// Saved as a JPEG at output_path.
static void plot_reflectance(
		const Pixel& I,
		const std::vector<Eigen::Vector3f>& L,
		const brdf::BrdfFitResult& result,
		float light_intensity,
		const QString& output_path)
{
	const Eigen::Vector3f V(0.0f, 0.0f, 1.0f);
	Eigen::Vector3f N_fit = result.normal.normalized();

	// X-axis parametrization: half-angle θ_h = arccos(H · N), where H = normalize(L+V).
	// θ_h = 0 when H = N, i.e. when light reflects directly toward the viewer (specular peak).

	// Collect measured (half_angle_deg, grey) pairs
	std::vector<std::pair<float,float>> samples;
	for (size_t k = 0; k < I.size(); ++k) {
		Eigen::Vector3f H = (L[k] + V).normalized();
		float cos_h = std::clamp(H.dot(N_fit), -1.0f, 1.0f);
		float angle  = std::acos(cos_h) * 180.0f / float(M_PI);
		float grey   = 0.2126f * I[k].r + 0.7152f * I[k].g + 0.0722f * I[k].b;
		samples.push_back({angle, grey});
	}

	// Build fitted curve: sweep the half-angle θ_h from 0 to 90°.
	// For each θ_h, reconstruct L by rotating H away from N in a fixed tangent plane,
	// then L = 2*(H·V)*H - V (reflect V about H).
	// Tangent direction: use the component of V perpendicular to N (falls back to X if V ∥ N).
	Eigen::Vector3f T = V - N_fit * N_fit.dot(V);
	if (T.norm() < 1e-4f) T = Eigen::Vector3f(1.0f, 0.0f, 0.0f);
	T.normalize();

	const int CURVE_STEPS = 200;
	Eigen::Matrix<float,3,1> baseColor = result.albedo;

	std::vector<std::pair<float,float>> curve;
	curve.reserve(CURVE_STEPS + 1);
	for (int s = 0; s <= CURVE_STEPS; ++s) {
		float theta_h = s * float(M_PI / 2.0) / CURVE_STEPS; // half-angle 0..90°
		Eigen::Vector3f H = std::cos(theta_h) * N_fit + std::sin(theta_h) * T;
		H.normalize();
		Eigen::Vector3f Ls = 2.0f * H.dot(V) * H - V; // reflect V about H
		if (Ls.z() <= 0.0f) break;                      // light below horizon — stop
		Eigen::Matrix<float,3,1> lc(light_intensity, light_intensity, light_intensity);
		Eigen::Vector3f pred = brdf::eval_gltf(N_fit, Ls, V, baseColor, result.metallic, result.roughness, lc);
		float grey = 0.2126f * pred.x() + 0.7152f * pred.y() + 0.0722f * pred.z();
		curve.push_back({theta_h * 180.0f / float(M_PI), grey});
	}

	// Determine y-axis range
	float y_max = 0.01f;
	for (auto& s : samples) y_max = std::max(y_max, s.second);
	for (auto& c : curve)   y_max = std::max(y_max, c.second);
	y_max *= 1.1f;

	// Canvas
	const int W = 640, H = 400;
	const int PAD_L = 55, PAD_R = 20, PAD_T = 20, PAD_B = 45;
	const int PW = W - PAD_L - PAD_R;
	const int PH = H - PAD_T - PAD_B;

	QImage img(W, H, QImage::Format_RGB888);
	img.fill(Qt::white);
	QPainter p(&img);
	p.setRenderHint(QPainter::Antialiasing);

	// Coordinate helpers (angle in [0,90], value in [0, y_max])
	auto px = [&](float angle) { return PAD_L + int(angle / 90.0f * PW); };
	auto py = [&](float val)   { return PAD_T + PH - int(val / y_max * PH); };

	// Grid lines & axis labels
	p.setPen(QPen(QColor(220,220,220), 1));
	for (int tick = 0; tick <= 4; ++tick) {
		int yv = PAD_T + tick * PH / 4;
		p.drawLine(PAD_L, yv, PAD_L + PW, yv);
		for (int xt = 0; xt <= 9; ++xt)
			p.drawLine(PAD_L + xt * PW / 9, PAD_T, PAD_L + xt * PW / 9, PAD_T + PH);
	}

	// Axes
	p.setPen(QPen(Qt::black, 2));
	p.drawRect(PAD_L, PAD_T, PW, PH);

	// Tick labels
	p.setFont(QFont("Sans", 8));
	for (int xt = 0; xt <= 9; ++xt) {
		int angle = xt * 10;
		p.drawText(PAD_L + xt * PW / 9 - 10, PAD_T + PH + 14, 20, 14,
				   Qt::AlignCenter, QString::number(angle));
	}
	for (int tick = 0; tick <= 4; ++tick) {
		int yv = PAD_T + tick * PH / 4;
		float val = y_max * (1.0f - float(tick) / 4);
		p.drawText(2, yv - 7, PAD_L - 4, 14, Qt::AlignRight | Qt::AlignVCenter,
				   QString::number(double(val), 'f', 2));
	}
	p.drawText(PAD_L, PAD_T + PH + 28, PW, 14, Qt::AlignCenter, "Half-angle θ_h (deg) — 0 = specular peak");

	// Fitted curve (dark grey)
	p.setPen(QPen(QColor(60,60,60), 2));
	for (int s = 1; s < (int)curve.size(); ++s)
		p.drawLine(px(curve[s-1].first), py(curve[s-1].second),
				   px(curve[s].first),   py(curve[s].second));

	// Measured samples (mid-grey dots)
	p.setPen(QPen(QColor(150,150,150), 1));
	p.setBrush(QColor(180,180,180));
	for (auto& s : samples)
		p.drawEllipse(QPoint(px(s.first), py(s.second)), 3, 3);

	p.end();
	img.save(output_path, "jpg", 90);
}

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
		// Open brute-force comparison file (only used when compute_brdf is true)
		std::ofstream brute_file;
		QMutex brute_mutex;
		QString plot_dir;
		if (parameters.compute_brdf) {
			QString brute_path = destination.filePath("brute_force_comparison.txt");
			brute_file.open(brute_path.toStdString());
			plot_dir = destination.filePath("reflectance_plots");
			QDir().mkpath(plot_dir);
		}

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

				BrdfWorker *task = new BrdfWorker(parameters, i, line, _n, _a, _r, _s, imageset, lens,
						&brute_file, &brute_mutex, plot_dir);
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

			// Build a linear-RGB → sRGB transform for display output
			cmsHPROFILE linear_profile = ICCProfiles::openLinearRGBProfile();
			cmsHPROFILE srgb_profile   = cmsCreate_sRGBProfile();
			cmsHTRANSFORM linear_to_srgb = cmsCreateTransform(
				linear_profile, TYPE_RGB_8,
				srgb_profile,   TYPE_RGB_8,
				INTENT_PERCEPTUAL, 0);
			cmsCloseProfile(linear_profile);
			cmsCloseProfile(srgb_profile);

			std::vector<Eigen::Vector3f> lights = imageset.lights();
			Eigen::Vector3f V(0.0f, 0.0f, 1.0f); // Orthographic view direction

			for (size_t light_idx = 0; light_idx < lights.size(); ++light_idx) {
				vector<uint8_t> render_data(width * height * 3);

				for (int y = 0; y < height; ++y) {
					for (int x = 0; x < width; ++x) {
                        if(x == width - 10 && y == height - 10 && light_idx == lights.size() -1)
                            x = x*1.0f;
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

                        //Eigen::Vector3f color = brdf::eval_ggx(N, lights[light_idx], V, A, R, S, 4.0f);
                        Eigen::Matrix<float, 3, 1> light_color(4.0f, 4.0f, 4.0f);
                        float metallic = 0.2126f * S.x() + 0.7152f * S.y() + 0.0722f * S.z();
                        Eigen::Vector3f color = brdf::eval_gltf(N, lights[light_idx], V, A, metallic, R, light_color);

						// Clamp and convert to 8-bit (still linear; will be converted via lcms below)
						render_data[rgb_idx + 0] = std::min(std::max((uint8_t)(color.x() * 255.0f), (uint8_t)0), (uint8_t)255);
						render_data[rgb_idx + 1] = std::min(std::max((uint8_t)(color.y() * 255.0f), (uint8_t)0), (uint8_t)255);
						render_data[rgb_idx + 2] = std::min(std::max((uint8_t)(color.z() * 255.0f), (uint8_t)0), (uint8_t)255);
					}
				}

				// Convert both original and render from linear to sRGB for display
				QImage orig = imageset.readImageCropped(light_idx);
				for(int row = 0; row < height; ++row)
					cmsDoTransform(linear_to_srgb, orig.scanLine(row), orig.scanLine(row), width);
				cmsDoTransform(linear_to_srgb, render_data.data(), render_data.data(), size_t(width) * height);

				// Build side-by-side image: original (left) | rendering (right)
				QImage side_by_side(width * 2, height, QImage::Format_RGB888);
				for(int y = 0; y < height; ++y) {
					memcpy(side_by_side.scanLine(y),              orig.scanLine(y),           width * 3);
					memcpy(side_by_side.scanLine(y) + width * 3, render_data.data() + y * width * 3, width * 3);
				}

				// Save side-by-side image
				QString render_path = QDir(testFolder).filePath(QString("render_%1.jpg").arg(light_idx, 3, 10, QChar('0')));
				side_by_side.save(render_path, "jpg", parameters.quality);
			}

			if(linear_to_srgb)
				cmsDeleteTransform(linear_to_srgb);

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
		// Initialize normal: use the light direction of the brightest sample (max luminance).
		// (Old approach: Lambertian photometric stereo — left for reference)
		//Eigen::VectorXf I_lum(p.size());
		//Eigen::MatrixXf L_mat(p.size(), 3);
		//for (size_t j = 0; j < p.size(); j++) {
		//	I_lum(j) = 0.2126f * p[j].r + 0.7152f * p[j].g + 0.0722f * p[j].b;
		//	L_mat.row(j) = L[j];
		//}
		//Eigen::Vector3f init_normal = brdf::simple_lambertian_photometric_stereo(I_lum, L_mat);
		//init_normal.normalize();

		size_t max_idx = 0;
		float max_lum = -1.0f;
		for (size_t j = 0; j < p.size(); j++) {
			float lum = 0.2126f * p[j].r + 0.7152f * p[j].g + 0.0722f * p[j].b;
			if (lum > max_lum) { max_lum = lum; max_idx = j; }
		}
		Eigen::Vector3f init_normal = (L[max_idx] + Eigen::Vector3f(0.0f, 0.0f, 1.0f)).normalized();
		
		// Handle invalid or negative backward-facing normals
		if (std::isnan(init_normal.x()) || init_normal.z() < 0) {
			init_normal = Eigen::Vector3f(0.0f, 0.0f, 1.0f);
		}

		// Optimize
		brdf::BrdfFitResult result = brdf::optimize_brdf_pixel(
                    p, L, init_normal, init_albedo_vec, 0.3f, 4.0f, false, true); // default rough=0.3, light_intensity=1.0

		// Brute-force comparison for 1 in every 100 pixels
        if (brute_out && brute_mutex) {
			int global_idx = row * (int)m_Row.size() + (int)i;
			if (global_idx % 100 == 0) {
				std::ostringstream oss;
                brdf::brdf_bruteforce_compare(p, L, result, 4.0f, m_Row[i].x, m_Row[i].y, oss, 0.1f, false, true);
				if (!plot_dir.isEmpty()) {
					QString plot_path = plot_dir + QString("/plot_%1_%2.jpg")
						.arg(m_Row[i].y, 5, 10, QChar('0'))
						.arg(m_Row[i].x, 5, 10, QChar('0'));
					plot_reflectance(p, L, result, 4.0f, plot_path);
				}
				QMutexLocker lk(brute_mutex);
				*brute_out << oss.str();
				brute_out->flush();
			}
		}

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
			uint8_t m = std::clamp(result.metallic * 255.0f, 0.0f, 255.0f);
			specular[i*3 + 0] = m;
			specular[i*3 + 1] = m;
			specular[i*3 + 2] = m;
		}
	}
}
