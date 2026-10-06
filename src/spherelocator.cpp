#include "spherelocator.h"

#include <QImage>
#include <QLineF>

#include <Eigen/Dense>

#include <algorithm>
#include <random>
#include <cmath>

using namespace std;

vector<QPointF> SphereLocator::Circle::points(int n) const {
	vector<QPointF> result;
	for(int i = 0; i < n; i++) {
		double a = 2*M_PI*i/n;
		result.push_back(center + radius*QPointF(cos(a), sin(a)));
	}
	return result;
}

SphereLocator::SphereLocator(QSize size) {
	image_size = size;
	scale = std::max(1.0f, std::max(size.width(), size.height())/(float)max_side);
	w = std::max(1, (int)round(size.width()/scale));
	h = std::max(1, (int)round(size.height()/scale));
}

static vector<uint8_t> toGray(const QImage &image) {
	QImage img = image.convertToFormat(QImage::Format_RGB32);
	vector<uint8_t> gray(size_t(img.width())*img.height());
	for(int y = 0; y < img.height(); y++) {
		const QRgb *line = (const QRgb *)img.constScanLine(y);
		for(int x = 0; x < img.width(); x++)
			gray[size_t(y)*img.width() + x] = (qRed(line[x]) + qGreen(line[x]) + qBlue(line[x]))/3;
	}
	return gray;
}

void SphereLocator::addImage(const QImage &img) {
	if(img.width() == w && img.height() == h)
		images.push_back(toGray(img));
	else
		images.push_back(toGray(img.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
}

static vector<uint8_t> percentileImage(const vector<vector<uint8_t>> &images, float fraction) {
	size_t n = images.size();
	size_t k = std::min(n - 1, size_t(fraction*n));
	size_t size = images[0].size();
	vector<uint8_t> result(size);
	vector<uint8_t> values(n);
	for(size_t p = 0; p < size; p++) {
		for(size_t i = 0; i < n; i++)
			values[i] = images[i][p];
		nth_element(values.begin(), values.begin() + k, values.end());
		result[p] = values[k];
	}
	return result;
}

//small spots much brighter than the median image.
vector<vector<SphereLocator::Spot>> SphereLocator::findSpots(const vector<uint8_t> &median) {
	vector<vector<Spot>> spots(images.size());
	size_t max_area = std::max<size_t>(4, size_t(0.0005*w*h));
	vector<int> diff(median.size());
	vector<bool> visited;
	vector<int> todo;
	for(size_t i = 0; i < images.size(); i++) {
		int max_diff = 0;
		for(size_t p = 0; p < median.size(); p++) {
			diff[p] = images[i][p] - median[p];
			max_diff = std::max(max_diff, diff[p]);
		}
		int threshold = std::max(int(0.15f*range), max_diff/2);
		visited.assign(median.size(), false);
		for(size_t p = 0; p < median.size(); p++) {
			if(visited[p] || diff[p] <= threshold)
				continue;
			//flood fill the spot
			double sx = 0, sy = 0, sw = 0;
			size_t area = 0;
			todo.push_back(int(p));
			visited[p] = true;
			while(todo.size()) {
				int q = todo.back();
				todo.pop_back();
				int x = q % w, y = q / w;
				sx += x*double(diff[q]);
				sy += y*double(diff[q]);
				sw += diff[q];
				area++;
				for(int k: { q - 1, q + 1, q - w, q + w }) {
					if(k < 0 || k >= int(diff.size()) || visited[k] || diff[k] <= threshold)
						continue;
					if(abs(k % w - x) > 1)
						continue;
					visited[k] = true;
					todo.push_back(k);
				}
			}
			if(area <= max_area)
				spots[i].push_back({ float(sx/sw), float(sy/sw) });
		}
	}
	return spots;
}

static bool fitCircle(const vector<QPointF> &points, QPointF &center, float &radius) {
	if(points.size() < 3)
		return false;
	Eigen::Matrix3d A = Eigen::Matrix3d::Zero();
	Eigen::Vector3d b = Eigen::Vector3d::Zero();
	for(const QPointF &p: points) {
		Eigen::Vector3d row(2*p.x(), 2*p.y(), 1);
		A += row*row.transpose();
		b += row*(p.x()*p.x() + p.y()*p.y());
	}
	Eigen::Vector3d s = A.ldlt().solve(b);
	double r2 = s[2] + s[0]*s[0] + s[1]*s[1];
	if(!std::isfinite(r2) || r2 <= 0)
		return false;
	center = QPointF(s[0], s[1]);
	radius = sqrt(r2);
	return true;
}

//circle through most points, inliers are within tolerance.
static bool ransacCircle(const vector<QPointF> &points, float tolerance, QPointF &center, float &radius, vector<QPointF> &inliers) {
	if(points.size() < 5)
		return false;
	std::mt19937 rng(0);
	std::uniform_int_distribution<size_t> pick(0, points.size() - 1);
	size_t best = 0;
	for(int iter = 0; iter < 500; iter++) {
		vector<QPointF> sample = { points[pick(rng)], points[pick(rng)], points[pick(rng)] };
		QPointF c;
		float r;
		if(!fitCircle(sample, c, r))
			continue;
		size_t count = 0;
		for(const QPointF &p: points)
			if(fabs(hypot(p.x() - c.x(), p.y() - c.y()) - r) < tolerance)
				count++;
		if(count > best) {
			best = count;
			center = c;
			radius = r;
		}
	}
	if(best < 5)
		return false;
	inliers.clear();
	for(const QPointF &p: points)
		if(fabs(hypot(p.x() - center.x(), p.y() - center.y()) - radius) < tolerance)
			inliers.push_back(p);
	return fitCircle(inliers, center, radius);
}

//along rays from the center: first crossing of the level halfway between the sphere and its surroundings,
//moved to the steepest point nearby. A circle is fitted to these points.
bool SphereLocator::traceOutline(const vector<uint8_t> &img, int width, int height, QPointF center, float spread, int nrays, float min_found, Circle &circle) {
	auto at = [&](int x, int y) { return img[size_t(y)*width + x]; };
	vector<uint8_t> inside, outside;
	int r = int(ceil(4*spread));
	for(int y = std::max(0, int(center.y()) - r); y <= std::min(height - 1, int(center.y()) + r); y++) {
		for(int x = std::max(0, int(center.x()) - r); x <= std::min(width - 1, int(center.x()) + r); x++) {
			float d = hypot(x - center.x(), y - center.y());
			if(d < 0.5*spread)
				inside.push_back(at(x, y));
			else if(d > 1.5*spread && d < 4*spread)
				outside.push_back(at(x, y));
		}
	}
	if(inside.empty() || outside.empty())
		return false;
	nth_element(inside.begin(), inside.begin() + inside.size()/2, inside.end());
	nth_element(outside.begin(), outside.begin() + outside.size()*3/4, outside.end());
	float in = inside[inside.size()/2];
	float out = outside[outside.size()*3/4];
	if(fabs(out - in) < 0.04f*range)
		return false;
	float level = (in + out)/2;
	float sign = out > in ? 1 : -1;

	auto sample = [&](float x, float y) {
		int x0 = int(x), y0 = int(y);
		float fx = x - x0, fy = y - y0;
		return (at(x0, y0)*(1 - fx) + at(x0 + 1, y0)*fx)*(1 - fy) + (at(x0, y0 + 1)*(1 - fx) + at(x0 + 1, y0 + 1)*fx)*fy;
	};
	vector<QPointF> points;
	vector<float> profile;
	const float step = 0.25f;
	int window = std::max(2, int(0.02f*spread/step)); //where to look for the steepest point around the crossing
	for(int k = 0; k < nrays; k++) {
		float a = 2*M_PI*k/nrays;
		float dx = cos(a), dy = sin(a);
		float start = 0.5f*spread;
		profile.clear();
		for(float d = start; d < 4*spread; d += step) {
			float x = center.x() + d*dx, y = center.y() + d*dy;
			if(x < 0 || y < 0 || x >= width - 1 || y >= height - 1)
				break;
			profile.push_back(sign*(sample(x, y) - level));
		}
		int cross = -1;
		for(size_t i = 1; i < profile.size(); i++) {
			if(profile[i - 1] < 0 && profile[i] >= 0) {
				cross = int(i);
				break;
			}
		}
		if(cross < 0)
			continue;
		//steepest point near the crossing, 1 pixel differences.
		int span = int(1/step);
		int best = -1;
		float best_slope = 0;
		for(int i = std::max(span, cross - window); i < std::min(int(profile.size()) - span, cross + window); i++) {
			float slope = profile[i + span] - profile[i - span];
			if(slope > best_slope) {
				best_slope = slope;
				best = i;
			}
		}
		if(best < 0)
			continue;
		float d = start + best*step;
		points.push_back(center + QPointF(d*dx, d*dy));
	}
	float tolerance = std::max(1.5f, 0.02f*spread);
	if(!ransacCircle(points, tolerance, circle.center, circle.radius, circle.border))
		return false;
	//the outline must surround the highlights and be found on enough rays.
	return circle.radius >= spread && circle.border.size() >= min_found*nrays;
}

//mean step across the circle (dark inside), higher is a better outline.
static float edgeScore(const vector<uint8_t> &img, int width, int height, QPointF center, float radius) {
	auto sample = [&](float x, float y) {
		int x0 = int(x), y0 = int(y);
		float fx = x - x0, fy = y - y0;
		const uint8_t *p = &img[size_t(y0)*width + x0];
		return (p[0]*(1 - fx) + p[1]*fx)*(1 - fy) + (p[width]*(1 - fx) + p[width + 1]*fx)*fy;
	};
	float d = std::max(1.0f, 0.01f*radius);
	float sum = 0;
	int count = 0;
	for(int k = 0; k < 360; k++) {
		float a = 2*M_PI*k/360;
		float dx = cos(a), dy = sin(a);
		float xo = center.x() + (radius + d)*dx, yo = center.y() + (radius + d)*dy;
		float xi = center.x() + (radius - d)*dx, yi = center.y() + (radius - d)*dy;
		if(std::min(xo, xi) < 0 || std::min(yo, yi) < 0 || std::max(xo, xi) >= width - 1 || std::max(yo, yi) >= height - 1)
			continue;
		sum += sample(xo, yo) - sample(xi, yi);
		count++;
	}
	return count ? sum/count : 0;
}

//the median image misses parts of the outline where shadows fall often, the bright image where the sphere rim reflects:
//keep the circle with the strongest edge in both.
bool SphereLocator::outline(const vector<uint8_t> &median, const vector<uint8_t> &bright, int width, int height, QPointF center, float spread, int nrays, float min_found, Circle &circle) {
	float best = 0;
	for(const vector<uint8_t> *img: { &median, &bright }) {
		Circle candidate;
		candidate.spread = spread;
		if(!traceOutline(*img, width, height, center, spread, nrays, min_found, candidate))
			continue;
		float score = edgeScore(median, width, height, candidate.center, candidate.radius) +
				edgeScore(bright, width, height, candidate.center, candidate.radius);
		if(score > best) {
			best = score;
			circle = candidate;
		}
	}
	return best > 0;
}

vector<SphereLocator::Circle> SphereLocator::locate() {
	vector<Circle> circles;
	int n = int(images.size());
	if(n < 3)
		return circles;

	vector<uint8_t> median = percentileImage(images, 0.5f);
	vector<uint8_t> bright = percentileImage(images, 0.9f);
	//thresholds are relative to the brightest lit areas, linear images can be very dark.
	vector<uint8_t> sorted = bright;
	nth_element(sorted.begin(), sorted.begin() + sorted.size()*99/100, sorted.end());
	range = std::max(20, int(sorted[sorted.size()*99/100]));
	vector<vector<Spot>> spots = findSpots(median);

	//pixels with a spot nearby in most images.
	struct Candidate { int count; int r; int x, y; };
	vector<Candidate> candidates;
	vector<uint16_t> count(size_t(w)*h);
	vector<int> stamp(size_t(w)*h);
	for(int r: { max_side/256, max_side/128, max_side/64, max_side/32 }) {
		std::fill(count.begin(), count.end(), 0);
		std::fill(stamp.begin(), stamp.end(), -1);
		for(int i = 0; i < n; i++) {
			for(Spot &s: spots[i]) {
				int cx = int(s.x), cy = int(s.y);
				for(int y = std::max(0, cy - r); y <= std::min(h - 1, cy + r); y++) {
					for(int x = std::max(0, cx - r); x <= std::min(w - 1, cx + r); x++) {
						if((x - cx)*(x - cx) + (y - cy)*(y - cy) > r*r)
							continue;
						size_t p = size_t(y)*w + x;
						if(stamp[p] != i) {
							stamp[p] = i;
							count[p]++;
						}
					}
				}
			}
		}
		for(int y = 0; y < h; y++)
			for(int x = 0; x < w; x++)
				if(count[size_t(y)*w + x] >= 0.6*n)
					candidates.push_back({ count[size_t(y)*w + x], r, x, y });
	}
	std::sort(candidates.begin(), candidates.end(), [](const Candidate &a, const Candidate &b) {
		return a.count > b.count || (a.count == b.count && a.r < b.r);
	});

	vector<QPointF> tried;
	for(Candidate &c: candidates) {
		QPointF pos(c.x, c.y);
		bool skip = false;
		for(Circle &circle: circles)
			if(hypot(pos.x() - circle.center.x(), pos.y() - circle.center.y()) < circle.radius)
				skip = true;
		for(QPointF &t: tried)
			if(hypot(pos.x() - t.x(), pos.y() - t.y()) < c.r)
				skip = true;
		if(skip)
			continue;
		tried.push_back(pos);

		//highlights of this sphere, the closest to the candidate in each image.
		vector<QPointF> group;
		for(auto &image_spots: spots) {
			float closest = 2*c.r;
			QPointF spot;
			for(Spot &s: image_spots) {
				float d = hypot(s.x - c.x, s.y - c.y);
				if(d <= closest) {
					closest = d;
					spot = QPointF(s.x, s.y);
				}
			}
			if(closest < 2*c.r)
				group.push_back(spot);
		}
		QPointF center(0, 0);
		for(QPointF &p: group)
			center += p;
		center /= group.size();
		vector<float> dist;
		for(QPointF &p: group)
			dist.push_back(hypot(p.x() - center.x(), p.y() - center.y()));
		nth_element(dist.begin(), dist.begin() + dist.size()*9/10, dist.end());
		float spread = std::max(3.0f, dist[dist.size()*9/10]);

		Circle circle;
		circle.spread = spread;
		if(!outline(median, bright, w, h, center, spread, 120, 0.4f, circle))
			continue;
		bool overlaps = false;
		for(Circle &other: circles)
			if(hypot(circle.center.x() - other.center.x(), circle.center.y() - other.center.y()) < circle.radius + other.radius)
				overlaps = true;
		if(!overlaps)
			circles.push_back(circle);
	}
	//to full image coordinates, pixel centers are at +0.5
	for(Circle &circle: circles) {
		circle.center = (circle.center + QPointF(0.5, 0.5))*scale;
		circle.radius *= scale;
		circle.spread *= scale;
		for(QPointF &p: circle.border)
			p = (p + QPointF(0.5, 0.5))*scale;
	}
	return circles;
}

QRect SphereLocator::cropRect(const Circle &circle) const {
	float pad = 1.6f*circle.radius;
	QRect rect(QPoint(int(circle.center.x() - pad), int(circle.center.y() - pad)), QPoint(int(circle.center.x() + pad), int(circle.center.y() + pad)));
	return rect.intersected(QRect(QPoint(0, 0), image_size));
}

bool SphereLocator::refine(Circle &circle, const vector<vector<uint8_t>> &grays) {
	QRect rect = cropRect(circle);
	vector<uint8_t> median = percentileImage(grays, 0.5f);
	vector<uint8_t> bright = percentileImage(grays, 0.9f);
	Circle refined;
	refined.spread = circle.spread;
	QPointF offset = rect.topLeft() + QPointF(0.5, 0.5);
	//full resolution edges are noisier: accept fewer rays, but close to the coarse circle.
	if(!outline(median, bright, rect.width(), rect.height(), circle.center - offset, circle.spread, 180, 0.25f, refined))
		return false;
	//keep the coarse circle if its edge is stronger.
	QPointF coarse = circle.center - offset;
	if(edgeScore(median, rect.width(), rect.height(), coarse, circle.radius) + edgeScore(bright, rect.width(), rect.height(), coarse, circle.radius) >=
		edgeScore(median, rect.width(), rect.height(), refined.center, refined.radius) + edgeScore(bright, rect.width(), rect.height(), refined.center, refined.radius))
		return false;
	refined.center += offset;
	if(QLineF(refined.center, circle.center).length() > 0.1*circle.radius || fabs(refined.radius - circle.radius) > 0.1*circle.radius)
		return false;
	for(QPointF &p: refined.border)
		p += offset;
	circle = refined;
	return true;
}

vector<SphereLocator::Circle> SphereLocator::run(int n, Loader load, std::function<bool(QString stage, int percent)> *callback) {
	auto progressed = [&](int percent) {
		if(callback && !(*callback)("Locating spheres", percent))
			throw QString("Cancelled.");
	};
	for(int i = 0; i < n; i++) {
		QImage img = load(i, QSize(w, h), QRect());
		if(img.isNull())
			throw QString("Failed loading image %1").arg(i);
		addImage(img);
		progressed(70*(i + 1)/n);
	}
	vector<Circle> circles = locate();

	//refine on full resolution crops of all the images.
	for(size_t c = 0; c < circles.size(); c++) {
		QRect rect = cropRect(circles[c]);
		vector<vector<uint8_t>> crops;
		for(int i = 0; i < n; i++) {
			QImage crop = load(i, QSize(), rect);
			if(crop.size() != rect.size())
				throw QString("Failed loading image %1").arg(i);
			crops.push_back(toGray(crop));
			progressed(70 + int(30*(c*n + i + 1)/(circles.size()*n)));
		}
		refine(circles[c], crops);
	}
	return circles;
}
