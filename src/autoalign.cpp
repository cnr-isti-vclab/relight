#include "autoalign.h"
#include "normals/pocketfft.h"

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

//phase correlation: shift to apply to b to match a, with subpixel peak.
static QPointF correlate(const vector<Complex> &a, const vector<Complex> &b, int side) {
	vector<Complex> r(a.size());
	for(size_t i = 0; i < a.size(); i++) {
		Complex c = a[i]*conj(b[i]);
		float m = abs(c);
		r[i] = m > 1e-12f ? c/m : Complex(0, 0);
	}
	fft(r, side, false);
	size_t best = 0;
	for(size_t i = 0; i < r.size(); i++)
		if(r[i].real() > r[best].real())
			best = i;
	int x = int(best % side), y = int(best / side);
	auto at = [&](int px, int py) { return r[size_t((py + side) % side)*side + (px + side) % side].real(); };
	auto peak = [](float m, float z, float p) {
		float d = m - 2*z + p;
		return d == 0 ? 0.0f : 0.5f*(m - p)/d;
	};
	float dx = peak(at(x - 1, y), at(x, y), at(x + 1, y));
	float dy = peak(at(x, y - 1), at(x, y), at(x, y + 1));
	return QPointF((x < side/2 ? x : x - side) + dx, (y < side/2 ? y : y - side) + dy);
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
