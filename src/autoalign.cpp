#include "autoalign.h"
#include "normals/pocketfft.h"

#include <Eigen/Core>
#include <Eigen/Sparse>
#include <Eigen/SparseCholesky>

#include <QImage>
#include <QLineF>

#include <algorithm>
#include <complex>
#include <cmath>

using namespace std;
typedef complex<float> Complex;

AutoAlign::AutoAlign(QSize size) {
	image_size = size;
	scale = std::max(1.0f, std::max(size.width(), size.height())/(float)max_side);
	w = std::max(1, (int)round(size.width()/scale));
	h = std::max(1, (int)round(size.height()/scale));
	side = std::max(32, std::min(max_side/6, std::min(w, h)));
}

//gradient magnitude of the gray image.
static vector<float> gradient(const QImage &image, int w, int h) {
	QImage img = image.convertToFormat(QImage::Format_Grayscale8);
	if(img.width() != w || img.height() != h)
		img = img.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	vector<float> grad(size_t(w)*h, 0.0f);
	for(int y = 1; y < h - 1; y++) {
		const uchar *up = img.constScanLine(y - 1);
		const uchar *down = img.constScanLine(y + 1);
		const uchar *line = img.constScanLine(y);
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

static QPointF peakShift(const vector<float> &surface, int x, int y, int side) {
	auto at = [&](int px, int py) { return surface[size_t((py + side) % side)*side + (px + side) % side]; };
	auto peak = [](float m, float z, float p) {
		float d = m - 2*z + p;
		return d == 0 ? 0.0f : 0.5f*(m - p)/d;
	};
	float dx = peak(at(x - 1, y), at(x, y), at(x + 1, y));
	float dy = peak(at(x, y - 1), at(x, y), at(x, y + 1));
	return QPointF((x < side/2 ? x : x - side) + dx, (y < side/2 ? y : y - side) + dy);
}

//phase correlation: shift to apply to b to match a, with subpixel peak.
static QPointF correlate(const vector<Complex> &a, const vector<Complex> &b, int side) {
	vector<float> surface = correlationSurface(a, b, side);
	size_t best = 0;
	for(size_t i = 0; i < surface.size(); i++)
		if(surface[i] > surface[best])
			best = i;
	int x = int(best % side), y = int(best / side);
	return peakShift(surface, x, y, side);
}

static vector<AutoAlignPeak> correlatePeaks(
	const vector<Complex> &a,
	const vector<Complex> &b,
	int side,
	int max_peaks
) {
	vector<float> surface = correlationSurface(a, b, side);
	vector<size_t> maxima;
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
			if(local_maximum)
				maxima.push_back(size_t(y)*side + x);
		}
	}
	sort(maxima.begin(), maxima.end(), [&](size_t a_idx, size_t b_idx) {
		if(surface[a_idx] != surface[b_idx])
			return surface[a_idx] > surface[b_idx];
		return a_idx < b_idx;
	});

	vector<AutoAlignPeak> peaks;
	peaks.reserve(std::min<size_t>(max_peaks, maxima.size()));
	for(size_t i = 0; i < std::min<size_t>(max_peaks, maxima.size()); i++) {
		size_t index = maxima[i];
		int x = int(index % side), y = int(index / side);
		peaks.push_back({-peakShift(surface, x, y, side), surface[index]});
	}
	return peaks;
}

static float median(vector<float> v) {
	nth_element(v.begin(), v.begin() + v.size()/2, v.end());
	return v[v.size()/2];
}

vector<QPointF> AutoAlign::run(int n, Loader load, std::function<bool(QString stage, int percent)> *callback) {
	if(n < 2)
		return vector<QPointF>(n, QPointF(0, 0));

	vector<float> window(side);
	for(int i = 0; i < side; i++)
		window[i] = 0.5f - 0.5f*cos(2*M_PI*i/(side - 1));

	vector<QPoint> tiles;
	for(int y = 0; y + side <= h; y += side/2)
		for(int x = 0; x + side <= w; x += side/2)
			tiles.push_back(QPoint(x, y));
	if(tiles.empty())
		throw QString("Images too small to align.");

	auto loadGradient = [&](int i) {
		QImage img = load(i, QSize(w, h));
		if(img.isNull())
			throw QString("Failed loading image %1").arg(i);
		return gradient(img, w, h);
	};

	vector<float> grad = loadGradient(0);
	vector<vector<Complex>> reference;
	for(QPoint &t: tiles)
		reference.push_back(tileSpectrum(grad, w, t.x(), t.y(), side, window));

	//shifts[i][j]: shift of tile j of image i.
	vector<vector<QPointF>> shifts(n, vector<QPointF>(tiles.size(), QPointF(0, 0)));
	for(int i = 1; i < n; i++) {
		grad = loadGradient(i);
		for(size_t j = 0; j < tiles.size(); j++)
			shifts[i][j] = correlate(reference[j], tileSpectrum(grad, w, tiles[j].x(), tiles[j].y(), side, window), side);
		if(callback && !(*callback)("Aligning images", 100*(i + 1)/n))
			throw QString("Cancelled.");
	}

	//distance of each tile from the median shift of its image, the most consistent tiles are used.
	vector<float> distance(tiles.size(), 0.0f);
	for(int i = 1; i < n; i++) {
		vector<float> xs, ys;
		for(QPointF &s: shifts[i]) {
			xs.push_back(s.x());
			ys.push_back(s.y());
		}
		QPointF m(median(xs), median(ys));
		for(size_t j = 0; j < tiles.size(); j++)
			distance[j] += QLineF(shifts[i][j], m).length();
	}
	vector<int> order(tiles.size());
	for(size_t j = 0; j < order.size(); j++)
		order[j] = int(j);
	sort(order.begin(), order.end(), [&](int a, int b) { return distance[a] < distance[b]; });
	order.resize(std::min<size_t>(3, order.size()));

	vector<QPointF> offsets(n, QPointF(0, 0));
	for(int i = 1; i < n; i++) {
		vector<float> xs, ys;
		for(int j: order) {
			xs.push_back(shifts[i][j].x());
			ys.push_back(shifts[i][j].y());
		}
		offsets[i] = -QPointF(median(xs), median(ys))*scale;
	}
	QPoint t = tiles[order[0]];
	region = QRect(int(t.x()*scale), int(t.y()*scale), int(side*scale), int(side*scale)).intersected(QRect(QPoint(0, 0), image_size));
	return offsets;
}

vector<AutoAlignCorrespondence> AutoAlign::runRegion(
	QRect selected_region,
	int n,
	RegionLoader load,
	std::function<bool(QString stage, int percent)> *callback
) {
	QRect image_bounds(QPoint(0, 0), image_size);
	if(selected_region.isEmpty() || !image_bounds.contains(selected_region))
		throw QString("Selected alignment region is outside the image.");
	if(selected_region.width() != selected_region.height())
		throw QString("Selected alignment region must be square.");
	if(selected_region.width() < 3)
		throw QString("Selected alignment region is too small.");

	if(n < 2)
		return {};

	int region_side = selected_region.width();
	vector<float> window(region_side);
	for(int i = 0; i < region_side; i++)
		window[i] = 0.5f - 0.5f*cos(2*M_PI*i/(region_side - 1));

	vector<vector<Complex>> spectra;
	spectra.reserve(size_t(n));
	for(int i = 0; i < n; i++) {
		QImage image = load(i, selected_region);
		if(image.isNull() || image.size() != selected_region.size())
			throw QString("Failed loading selected region from image %1").arg(i);
		vector<float> grad = gradient(image, region_side, region_side);
		spectra.push_back(tileSpectrum(grad, region_side, 0, 0, region_side, window));
		if(callback && !(*callback)("Preparing selected regions", 50*(i + 1)/n))
			throw QString("Cancelled.");
	}

	size_t edge_count = 0;
	for(size_t first = 0; first < size_t(n); first++)
		for(size_t step = 1; first + step < size_t(n); step *= 2)
			edge_count++;

	vector<AutoAlignCorrespondence> correspondences;
	correspondences.reserve(edge_count);
	size_t completed = 0;
	for(size_t first = 0; first < size_t(n); first++) {
		for(size_t step = 1; first + step < size_t(n); step *= 2) {
			size_t second = first + step;
			correspondences.push_back({
				int(first),
				int(second),
				correlatePeaks(spectra[first], spectra[second], region_side, 4)
			});
			completed++;
			if(callback && !(*callback)(
				"Finding pair candidates",
				50 + int(50*completed/edge_count)
			))
				throw QString("Cancelled.");
		}
	}
	return correspondences;
}

vector<QPointF> AutoAlign::solveTranslations(
	int n,
	const vector<AutoAlignCorrespondence> &correspondences
) const {
	if(n < 2)
		throw QString("At least two images are required to solve translations.");
	vector<QPointF> translations(size_t(n), QPointF(0, 0));

	struct Edge {
		int first;
		int second;
		QPointF offset;
	};
	vector<Edge> edges;
	edges.reserve(correspondences.size());
	for(const AutoAlignCorrespondence &correspondence: correspondences) {
		if(correspondence.first < 0 || correspondence.second >= n ||
			correspondence.first >= correspondence.second)
			throw QString("Invalid image-pair correspondence.");
		if(!correspondence.peaks.empty())
			edges.push_back({correspondence.first, correspondence.second, correspondence.peaks.front().offset});
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
		translations[size_t(i)] = QPointF(x[i - 1], y[i - 1]);
	return translations;
}
