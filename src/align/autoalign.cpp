#include "align/autoalign.h"
#include "normals/pocketfft.h"
#include "../image_decoder.h"

#include <Eigen/Core>
#include <Eigen/Sparse>
#include <Eigen/SparseCholesky>

#include <QColor>
#include <QImage>
#include <QSaveFile>
#include <QTextStream>

#include <algorithm>
#include <atomic>
#include <complex>
#include <cmath>
#include <exception>
#include <limits>
#include <utility>

using namespace std;
typedef complex<float> Complex;

AutoAlign::AutoAlign(QSize size) {
	w = size.width();
	h = size.height();
}

// Compute gradient values only inside a candidate patch. Neighbor pixels just
// outside the patch are sampled for the central-difference stencil.
static vector<float> gradientPatch(const uint8_t *gray, int width, int height,
	int x0, int y0, int side) {
	vector<float> grad(size_t(side)*side, 0.0f);
	for(int y = 0; y < side; ++y) {
		const int image_y = y0 + y;
		if(image_y <= 0 || image_y >= height - 1)
			continue;
		const uint8_t *up = gray + size_t(image_y - 1) * size_t(width);
		const uint8_t *down = gray + size_t(image_y + 1) * size_t(width);
		const uint8_t *line = gray + size_t(image_y) * size_t(width);
		for(int x = 0; x < side; ++x) {
			const int image_x = x0 + x;
			if(image_x <= 0 || image_x >= width - 1)
				continue;
			const float dx = float(line[image_x + 1]) - float(line[image_x - 1]);
			const float dy = float(down[image_x]) - float(up[image_x]);
			grad[size_t(y)*side + x] = sqrt(dx*dx + dy*dy);
		}
	}
	return grad;
}

static vector<float> gradient(const uint8_t *image, int w, int h) {
	vector<float> grad(size_t(w)*h, 0.0f);
	for(int y = 1; y < h - 1; y++) {
		const uint8_t *up = image + size_t(y - 1)*w;
		const uint8_t *down = image + size_t(y + 1)*w;
		const uint8_t *line = image + size_t(y)*w;
		for(int x = 1; x < w - 1; x++) {
			float dx = float(line[x + 1]) - float(line[x - 1]);
			float dy = float(down[x]) - float(up[x]);
			grad[size_t(y)*w + x] = sqrt(dx*dx + dy*dy);
		}
	}
	return grad;
}

static void fft(vector<Complex> &data, int side, bool forward) {
	pocketfft::shape_t shape = { size_t(side), size_t(side) };
	pocketfft::stride_t stride = { ptrdiff_t(sizeof(Complex)), ptrdiff_t(side*sizeof(Complex)) };
	pocketfft::shape_t axes = { 0, 1 };
	pocketfft::c2c(shape, stride, stride, axes, forward, data.data(), data.data(), 1.0f);
}

//normalized and windowed tile, transformed.
static vector<Complex> tileSpectrum(const vector<float> &grad, int w, int x0, int y0, int side, const vector<float> &window) {
	vector<Complex> tile(size_t(side)*side);
	double sum = 0, sum2 = 0;
	for(int y = 0; y < side; y++) {
		for(int x = 0; x < side; x++) {
			float v = grad[size_t(y0 + y)*w + x0 + x];
			sum += v;
			sum2 += v*v;
		}
	}
	double n = double(side)*side;
	float mean = sum/n;
	float dev = sqrt(std::max(1e-6, sum2/n - mean*mean));
	for(int y = 0; y < side; y++)
		for(int x = 0; x < side; x++)
			tile[size_t(y)*side + x] = (grad[size_t(y0 + y)*w + x0 + x] - mean)/dev*window[y]*window[x];
	fft(tile, side, true);
	return tile;
}

static vector<float> correlationSurface(const vector<Complex> &a, const vector<Complex> &b, int side) {
	vector<Complex> r(a.size());
	for(size_t i = 0; i < a.size(); i++) {
		Complex c = a[i]*conj(b[i]);
		float m = abs(c);
		r[i] = m > 1e-12f ? c/m : Complex(0, 0);
	}
	fft(r, side, false);
	vector<float> surface(r.size());
	for(size_t i = 0; i < r.size(); i++)
		surface[i] = r[i].real();
	return surface;
}

static Eigen::Vector2f peakShift(const vector<float> &surface, int x, int y, int side) {
	auto at = [&](int px, int py) { return surface[size_t((py + side) % side)*side + (px + side) % side]; };
	auto peak = [](float m, float z, float p) {
		float d = m - 2*z + p;
		return d == 0 ? 0.0f : 0.5f*(m - p)/d;
	};
	float dx = peak(at(x - 1, y), at(x, y), at(x + 1, y));
	float dy = peak(at(x, y - 1), at(x, y), at(x, y + 1));
	return Eigen::Vector2f((x < side/2 ? x : x - side) + dx, (y < side/2 ? y : y - side) + dy);
}


static optional<AutoAlignPeak> correlatePeak(
	const vector<Complex> &a,
	const vector<Complex> &b,
	int side,
	int first_image,
	int second_image) {
	vector<float> surface = correlationSurface(a, b, side);
//for debug purposes
//	saveCorrelationSurface(surface, side, first_image, second_image);

	size_t best = 0;
	bool found = false;
	for(int y = 0; y < side; y++) {
		for(int x = 0; x < side; x++) {
			float value = surface[size_t(y)*side + x];
			bool local_maximum = true;
			for(int dy = -1; dy <= 1 && local_maximum; dy++) {
				for(int dx = -1; dx <= 1; dx++) {
					if(dx == 0 && dy == 0)
						continue;
					int px = (x + dx + side) % side;
					int py = (y + dy + side) % side;
					if(surface[size_t(py)*side + px] >= value) {
						local_maximum = false;
						break;
					}
				}
			}
			if(local_maximum && (!found || value > surface[best])) {
				best = size_t(y)*side + x;
				found = true;
			}
		}
	}
	if(!found)
		return std::nullopt;
	int x = int(best % side), y = int(best / side);
	return AutoAlignPeak{-peakShift(surface, x, y, side), surface[best]};
}

vector<QPointF> AutoAlign::run(const vector<QString> &image_paths,
	std::function<bool(QString stage, int percent)> *callback) {
	const int n = int(image_paths.size());
	if(n <= 0)
		return {};
	if(n == 1)
		return vector<QPointF>(1, QPointF(0, 0));

	// Choose an even patch size that fits comfortably within every input image.
	const int minimum_dimension = std::min(w, h);
	const int patch_side = std::min(side, (minimum_dimension / 3) & ~1);
	if(patch_side < 40 || w < patch_side || h < patch_side)
		throw QString("Images too small to align.");
	auto report = [&](QString stage, int percent) {
		return !callback || (*callback)(stage, std::clamp(percent, 0, 100));
	};

	// Generate 32 center-biased candidate patch locations using a golden-angle spiral.
	constexpr int candidate_count = 32;
	constexpr int selected_count = 10;
	const int max_x = w - patch_side;
	const int max_y = h - patch_side;
	const double center_x = max_x * 0.5;
	const double center_y = max_y * 0.5;
	const double golden_angle = M_PI * (3.0 - sqrt(5.0));
	vector<QPoint> candidates;
	candidates.reserve(candidate_count);
	for(int i = 0; i < candidate_count; ++i) {
		const double fraction = (i + 0.5) / candidate_count;
		const double radius = pow(fraction, 1.5);
		const double angle = i * golden_angle;
		QPoint point(
			int(lround(center_x + cos(angle) * center_x * radius)),
			int(lround(center_y + sin(angle) * center_y * radius))
		);
		if(std::find(candidates.begin(), candidates.end(), point) == candidates.end())
			candidates.push_back(point);
	}
	if(candidates.empty())
		throw QString("Could not select alignment regions.");

	// In parallel, load each image once and compute/cache gradients only for its
	// candidate patches; also record each patch's gradient energy for ranking.
	vector<vector<vector<float>>> candidate_gradients(candidates.size());
	for(auto &per_image: candidate_gradients)
		per_image.resize(size_t(n));
	vector<double> image_energy(size_t(n)*candidates.size(), 0.0);
	vector<QString> load_errors(static_cast<size_t>(n));
	atomic<bool> cancelled(false);
	int completed_images = 0;
	#pragma omp parallel for schedule(dynamic)
	for(int image_index = 0; image_index < n; ++image_index) {
		if(cancelled.load())
			continue;
		try {
			const QString &path = image_paths[size_t(image_index)];
			const string filename = path.toStdString();

			ImageDecoder decoder;
			int image_width = 0, image_height = 0;
			if(!decoder.init(filename.c_str(), image_width, image_height))
				throw QString("Failed loading image: %1").arg(path);
			if(image_width != w || image_height != h)
				throw QString("Image dimensions do not match: %1").arg(path);
			vector<uint8_t> pixels(size_t(image_height) * decoder.rowSize(true));
			if(decoder.readRows(image_height, pixels.data(), true) != size_t(image_height))
				throw QString("Failed reading image: %1").arg(path);

			for(size_t candidate_index = 0; candidate_index < candidates.size(); ++candidate_index) {
				const QPoint &point = candidates[candidate_index];
				vector<float> patch_gradient = gradientPatch(
					pixels.data(), image_width, image_height,
					point.x(), point.y(), patch_side);
				double energy = 0.0;
				for(float value: patch_gradient)
					energy += value;
				candidate_gradients[candidate_index][size_t(image_index)] = std::move(patch_gradient);
				image_energy[size_t(image_index)*candidates.size() + candidate_index] = energy;
			}
		} catch(const QString &message) {
			load_errors[size_t(image_index)] = message;
			cancelled.store(true);
		} catch(...) {
			load_errors[size_t(image_index)] = QString("Unknown error loading image %1").arg(image_index);
			cancelled.store(true);
		}
		#pragma omp critical(autoalign_progress)
		{
			++completed_images;
			if(!cancelled.load() && !report("Scoring candidate regions",
				15 * completed_images / n))
				cancelled.store(true);
		}
	}
	for(const QString &error: load_errors)
		if(!error.isEmpty())
			throw error;
	if(cancelled.load())
		throw QString("Cancelled.");

	// Score a candidate by its weakest gradient energy across all images, so every
	// selected patch has useful texture throughout the image set.
	vector<double> minimum_energy(candidates.size(), numeric_limits<double>::infinity());
	for(size_t candidate_index = 0; candidate_index < candidates.size(); ++candidate_index)
		for(int image_index = 0; image_index < n; ++image_index)
			minimum_energy[candidate_index] = std::min(
				minimum_energy[candidate_index],
				image_energy[size_t(image_index)*candidates.size() + candidate_index]);

	vector<int> order(candidates.size());
	for(size_t i = 0; i < order.size(); ++i)
		order[i] = int(i);
	stable_sort(order.begin(), order.end(), [&](int a, int b) {
		return minimum_energy[size_t(a)] > minimum_energy[size_t(b)];
	});
	
	// Greedily keep the strongest candidates while excluding overlapping patches.
	vector<int> selected;
	selected.reserve(selected_count);
	for(int candidate: order) {
		const QPoint &point = candidates[size_t(candidate)];
		const bool overlaps = std::any_of(selected.begin(), selected.end(), [&](int other) {
			const QPoint &chosen = candidates[size_t(other)];
			return std::abs(point.x() - chosen.x()) < patch_side &&
				std::abs(point.y() - chosen.y()) < patch_side;
		});
		if(!overlaps)
			selected.push_back(candidate);
		if(selected.size() == selected_count)
			break;
	}
	order = std::move(selected);
	if(order.empty() || !std::isfinite(minimum_energy[size_t(order.front())]) ||
		minimum_energy[size_t(order.front())] <= 0.0)
		throw QString("Could not find textured regions suitable for alignment.");

	//for debug purposes
	//saveRegionSvg(candidates, order, patch_side, w, h);

	// Keep gradients only for selected patches and release all unselected candidates.
	vector<vector<vector<float>>> selected_gradients;
	selected_gradients.reserve(order.size());
	for(int candidate_index: order)
		selected_gradients.push_back(std::move(candidate_gradients[size_t(candidate_index)]));
	candidate_gradients.clear();
	candidate_gradients.shrink_to_fit();

	// Build spectra for each selected patch, correlate image pairs, and collect all
	// correspondences so the final image translations are solved jointly.
	vector<AutoAlignCorrespondence> all_correspondences;
	for(size_t selected_index = 0; selected_index < order.size(); ++selected_index) {
		const int selected_number = int(selected_index + 1);
		const int selected_total = int(order.size());
		beginRegion(QSize(patch_side, patch_side), n);
		vector<QString> spectrum_errors(static_cast<size_t>(n));
		#pragma omp parallel for
		for(int image_index = 0; image_index < n; ++image_index) {
			try {
				region_spectra[size_t(image_index)] =
					tileSpectrum(selected_gradients[selected_index][size_t(image_index)],
						patch_side, 0, 0, patch_side, region_window);
			} catch(const QString &message) {
				spectrum_errors[size_t(image_index)] = message;
			} catch(const std::exception &exception) {
				spectrum_errors[size_t(image_index)] = QString::fromUtf8(exception.what());
			} catch(...) {
				spectrum_errors[size_t(image_index)] =
					QString("Unknown error preparing region %1 for image %2")
						.arg(selected_number).arg(image_index);
			}
		}
		for(const QString &error: spectrum_errors)
			if(!error.isEmpty())
				throw error;

		vector<AutoAlignCorrespondence> correspondences = findCandidates();
		all_correspondences.insert(all_correspondences.end(),
			correspondences.begin(), correspondences.end());
		const int progress = 15 + int(75.0 * selected_number / selected_total);
		if(!report(QString("Selected region %1/%2")
			.arg(selected_number).arg(selected_total), progress))
			throw QString("Cancelled.");
	}

	// Robustly solve the combined correspondence graph with image 0 fixed at zero.
	if(!report("Solving combined region translations", 95))
		throw QString("Cancelled.");
	vector<Eigen::Vector2f> translations = solveTranslations(n, all_correspondences);
	vector<QPointF> offsets(size_t(n), QPointF(0, 0));
	for(int i = 1; i < n; ++i) {
		const Eigen::Vector2f &translation = translations[size_t(i)];
		offsets[size_t(i)] = QPointF(translation.x(), translation.y());
	}
	if(!report("Alignment complete", 100))
		throw QString("Cancelled.");
	return offsets;
}

void AutoAlign::beginRegion(QSize region_size, int image_count) {
	if(region_size.isEmpty())
		throw QString("Selected alignment region must not be empty.");
	if(region_size.width() != region_size.height())
		throw QString("Selected alignment region must be square.");
	if(region_size.width() % 2 != 0 || region_size.height() % 2 != 0)
		throw QString("Selected alignment region dimensions must be even.");
	if(image_count < 2)
		throw QString("At least two images are required to align.");

	region_side = region_size.width();
	region_image_count = image_count;
	region_window.resize(size_t(region_side));
	for(int i = 0; i < region_side; i++)
		region_window[size_t(i)] = 0.5f - 0.5f*cos(2*M_PI*i/(region_side - 1));

	region_spectra.resize(size_t(region_image_count));
}

void AutoAlign::provideImage(int image_index, const uint8_t *greyscale_image) {
	if(region_side == 0)
		throw QString("Begin an alignment region before providing images.");
	if(image_index < 0 || image_index >= region_image_count)
		throw QString("Image index is outside the selected alignment set.");

	vector<float> grad = gradient(greyscale_image, region_side, region_side);
	region_spectra[size_t(image_index)] = tileSpectrum(grad, region_side, 0, 0, region_side, region_window);
}

vector<AutoAlignCorrespondence> AutoAlign::findCandidates(
	std::function<bool(QString stage, int percent)> *callback) {
	if(region_side == 0)
		throw QString("Begin an alignment region before finding candidates.");

	int edge_count = 0;
	vector<int> first_offsets(region_image_count + 1);
	for(int first = 0; first < region_image_count; first++) {
		first_offsets[first] = edge_count;
		for(int step = 1; step < region_image_count - first;) {
			edge_count++;
			if(step > (region_image_count - first - 1)/2)
				break;
			step *= 2;
		}
	}
	first_offsets[region_image_count] = edge_count;

	vector<AutoAlignCorrespondence> correspondences(static_cast<size_t>(edge_count));
	atomic<bool> cancelled(false);
	atomic<bool> error_occurred(false);
	int completed = 0;
	#pragma omp parallel for
	for(int first = 0; first < region_image_count; first++) {
		int index = first_offsets[first];
		for(int step = 1; step < region_image_count - first;) {
			if(cancelled.load())
				break;
			const int second = first + step;
			try {
				correspondences[static_cast<size_t>(index)] = {
					first,
					second,
					correlatePeak(
						region_spectra[static_cast<size_t>(first)],
						region_spectra[static_cast<size_t>(second)],
						region_side,
						first + 1,
						second + 1)
				};
			} catch(...) {
				error_occurred.store(true);
				cancelled.store(true);
				break;
			}
			index++;
			if(callback) {
				#pragma omp critical(autoalign_progress)
				{
					if(!cancelled.load()) {
						completed++;
						if(!(*callback)(
							"Finding pair candidates",
							50 + static_cast<int>(50LL*completed/edge_count)
						))
							cancelled.store(true);
					}
				}
			}
			if(step > (region_image_count - first - 1)/2)
				break;
			step *= 2;
		}
	}
	if(error_occurred.load())
		throw QString("An error occurred while finding pair candidates.");
	if(cancelled.load())
		throw QString("Cancelled.");
	return correspondences;
}

vector<Eigen::Vector2f> AutoAlign::solveTranslations(
	int n,
	const vector<AutoAlignCorrespondence> &correspondences
) const {
	if(n < 2)
		throw QString("At least two images are required to solve translations.");
	vector<Eigen::Vector2f> translations(size_t(n), Eigen::Vector2f::Zero());

	struct Edge {
		int first;
		int second;
		Eigen::Vector2f offset;
	};
	vector<Edge> edges;
	edges.reserve(correspondences.size());
	for(const AutoAlignCorrespondence &correspondence: correspondences) {
		if(correspondence.first < 0 || correspondence.second >= n ||
			correspondence.first >= correspondence.second)
			throw QString("Invalid image-pair correspondence.");
		if(correspondence.peak)
			edges.push_back({correspondence.first, correspondence.second, correspondence.peak->offset});
	}
	if(edges.empty())
		throw QString("No valid phase-correlation peaks to solve translations.");

	const int unknowns = n - 1;
	vector<double> weights(edges.size(), 1.0);
	Eigen::VectorXd x = Eigen::VectorXd::Zero(unknowns);
	Eigen::VectorXd y = Eigen::VectorXd::Zero(unknowns);

	auto solveWeighted = [&]() {
		vector<Eigen::Triplet<double>> coefficients;
		coefficients.reserve(edges.size()*4);
		Eigen::VectorXd rhs_x = Eigen::VectorXd::Zero(unknowns);
		Eigen::VectorXd rhs_y = Eigen::VectorXd::Zero(unknowns);
		auto addDiagonal = [&](int image, double weight) {
			if(image > 0)
				coefficients.emplace_back(image - 1, image - 1, weight);
		};
		auto addRhs = [&](int image, double sign, const Edge &edge, double weight) {
			if(image > 0) {
				rhs_x[image - 1] += sign*weight*edge.offset.x();
				rhs_y[image - 1] += sign*weight*edge.offset.y();
			}
		};

		for(size_t k = 0; k < edges.size(); k++) {
			const Edge &edge = edges[k];
			double weight = weights[k];
			addDiagonal(edge.first, weight);
			addDiagonal(edge.second, weight);
			if(edge.first > 0 && edge.second > 0) {
				coefficients.emplace_back(edge.first - 1, edge.second - 1, -weight);
				coefficients.emplace_back(edge.second - 1, edge.first - 1, -weight);
			}
			addRhs(edge.first, -1.0, edge, weight);
			addRhs(edge.second, 1.0, edge, weight);
		}

		Eigen::SparseMatrix<double> normal(unknowns, unknowns);
		normal.setFromTriplets(coefficients.begin(), coefficients.end());
		Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> solver;
		solver.compute(normal);
		if(solver.info() != Eigen::Success)
			throw QString("Could not factor the translation system.");
		Eigen::VectorXd diagonal = solver.vectorD();
		if(diagonal.minCoeff() <= diagonal.maxCoeff()*1e-12)
			throw QString("The correspondence graph does not connect every image.");

		x = solver.solve(rhs_x);
		y = solver.solve(rhs_y);
		if(solver.info() != Eigen::Success || !x.allFinite() || !y.allFinite())
			throw QString("Could not solve the translation system.");
	};

	auto median = [](vector<double> values) {
		size_t middle = values.size()/2;
		nth_element(values.begin(), values.begin() + middle, values.end());
		return values[middle];
	};
	for(int iteration = 0; iteration < 5; iteration++) {
		solveWeighted();
		if(iteration == 4)
			break;

		vector<double> residual_x(edges.size());
		vector<double> residual_y(edges.size());
		for(size_t k = 0; k < edges.size(); k++) {
			const Edge &edge = edges[k];
			double first_x = edge.first > 0 ? x[edge.first - 1] : 0.0;
			double first_y = edge.first > 0 ? y[edge.first - 1] : 0.0;
			double second_x = edge.second > 0 ? x[edge.second - 1] : 0.0;
			double second_y = edge.second > 0 ? y[edge.second - 1] : 0.0;
			residual_x[k] = second_x - first_x - edge.offset.x();
			residual_y[k] = second_y - first_y - edge.offset.y();
		}

		double center_x = median(residual_x);
		double center_y = median(residual_y);
		vector<double> deviations;
		deviations.reserve(edges.size()*2);
		for(size_t k = 0; k < edges.size(); k++) {
			deviations.push_back(abs(residual_x[k] - center_x));
			deviations.push_back(abs(residual_y[k] - center_y));
		}
		double scale = std::max(1.0, 2.3849*1.4826*median(deviations));
		for(size_t k = 0; k < edges.size(); k++) {
			double residual = hypot(residual_x[k], residual_y[k]);
			double normalized = residual/scale;
			weights[k] = 1.0/(1.0 + normalized*normalized);
		}
	}

	for(int i = 1; i < n; i++)
		translations[size_t(i)] = Eigen::Vector2f(float(x[i - 1]), float(y[i - 1]));
	return translations;
}

static void saveCorrelationSurface(const vector<float> &surface, int side, int first, int second) {
	const int crop_side = 40;
	int crop_start = side/2 - crop_side/2;
	vector<float> centered(size_t(crop_side)*crop_side);
	// Adding half a side performs the quadrant swap, placing zero displacement at the crop center.
	for(int y = 0; y < crop_side; y++) {
		int source_y = (crop_start + y + side/2) % side;
		for(int x = 0; x < crop_side; x++) {
			int source_x = (crop_start + x + side/2) % side;
			centered[size_t(y)*crop_side + x] = surface[size_t(source_y)*side + source_x];
		}
	}

	auto range = minmax_element(centered.begin(), centered.end());
	float minimum = *range.first;
	float value_range = *range.second - minimum;
	QImage image(crop_side, crop_side, QImage::Format_RGB32);
	for(int y = 0; y < crop_side; y++) {
		for(int x = 0; x < crop_side; x++) {
			float value = value_range > 0
				? (centered[size_t(y)*crop_side + x] - minimum)/value_range
				: 0.5f;
			int hue = int((1.0f - value)*240.0f);
			image.setPixel(x, y, QColor::fromHsv(hue, 255, 255).rgb());
		}
	}

	QString filename = QString("correlation_%1_%2.png").arg(first).arg(second);
	if(!image.save(filename, "PNG"))
		throw QString("Could not save correlation surface to %1.").arg(filename);
}

static void saveRegionSvg(
	const vector<QPoint> &candidates,
	const vector<int> &selected,
	int side,
	int width, int height
) {
	static const char *colors[] = {
		"#e41a1c", "#377eb8", "#4daf4a", "#984ea3", "#ff7f00",
		"#a65628", "#f781bf", "#17becf", "#bcbd22", "#7f7f7f"
	};
	QSaveFile file("autoalign_regions.svg");
	if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
		throw QString("Could not write autoalign_regions.svg: %1").arg(file.errorString());

	QTextStream svg(&file);
	svg << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		<< "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width
		<< "\" height=\"" << height << "\" viewBox=\"0 0 "
		<< width<< " " << height << "\">\n"
		<< "<title>Automatic alignment candidate regions</title>\n"
		<< "<desc>Dashed gray squares show all candidate regions; numbered colored squares show the ten highest-energy regions selected for alignment.</desc>\n"
		<< "<rect x=\"0\" y=\"0\" width=\"100%\" height=\"100%\" fill=\"white\"/>\n"
		<< "<g fill=\"none\" stroke=\"#666666\" stroke-width=\"" << 1.0f
		<< "\" stroke-dasharray=\"" << 4.0f << " "
		<< 3.0f << "\" opacity=\"0.45\">\n";
	for(const QPoint &point: candidates) {
		svg << "<rect x=\"" << point.x() << "\" y=\"" << point.y()
			<< "\" width=\"" << side << "\" height=\"" << side << "\"/>\n";
	}
	svg << "</g>\n";
	const float stroke_width = std::max(2.0f, 2.0f);
	const float font_size = std::max(14.0f, 12.0f);
	for(size_t rank = 0; rank < selected.size(); ++rank) {
		const QPoint &point = candidates[size_t(selected[rank])];
		const float x = point.x();
		const float y = point.y();
		const float tile_side = side;
		svg << "<g fill=\"" << colors[rank % (sizeof(colors) / sizeof(colors[0]))]
			<< "\" fill-opacity=\"0.12\" stroke=\""
			<< colors[rank % (sizeof(colors) / sizeof(colors[0]))]
			<< "\" stroke-width=\"" << stroke_width << "\">\n"
			<< "<rect x=\"" << x << "\" y=\"" << y << "\" width=\"" << tile_side
			<< "\" height=\"" << tile_side << "\"/>\n"
			<< "</g>\n"
			<< "<text x=\"" << x + stroke_width << "\" y=\"" << y + font_size + stroke_width
			<< "\" font-family=\"sans-serif\" font-size=\"" << font_size
			<< "\" font-weight=\"bold\" fill=\"" << colors[rank % (sizeof(colors) / sizeof(colors[0]))]
			<< "\" stroke=\"white\" stroke-width=\"" << 1.0f
			<< "\" paint-order=\"stroke\">" << (rank + 1) << "</text>\n";
	}
	svg << "</svg>\n";
	if(!file.commit())
		throw QString("Could not save autoalign_regions.svg: %1").arg(file.errorString());
}
