#ifndef SPHERELOCATOR_H
#define SPHERELOCATOR_H

#include <vector>
#include <cstdint>
#include <functional>

#include <QPointF>
#include <QRect>
#include <QSize>
#include <QString>

class QImage;

/* Finds the reflective spheres in an RTI image set.
   Highlights that fall in the same small area in most images are grouped (one group per sphere),
   the outline is where the dark sphere ends, in the median image or in an image without shadows. */

class SphereLocator {
public:
	struct Circle {
		QPointF center;
		float radius = 0.0f;
		float spread = 0.0f; //radius of the area containing the highlights
		std::vector<QPointF> border; //points found on the outline

		//n points on the circle, to be used as sphere border (less than 5: Sphere::fit() fits a circle).
		std::vector<QPointF> points(int n = 4) const;
	};

	const int max_side = 1024; //images are downscaled to this size to find the spheres.

	//returns image i downscaled to size, or if clip is valid that part of the image at full resolution.
	typedef std::function<QImage(int i, QSize size, QRect clip)> Loader;

	SphereLocator(QSize image_size);
	//loads n images, locates and refines the spheres. Throws QString on failure or cancel.
	std::vector<Circle> run(int n, Loader load, std::function<bool(QString stage, int percent)> *callback = nullptr);

private:
	QSize image_size; //full size
	int w = 0, h = 0; //downscaled size
	float scale = 1.0f; //full size / downscaled size
	float range = 255; //gray level of the brightest lit areas
	std::vector<std::vector<uint8_t>> images; //downscaled gray images

	//image of the set, any size: it is downscaled.
	void addImage(const QImage &img);
	//spheres found in the downscaled images, in full image coordinates
	std::vector<Circle> locate();
	//refine circle on full resolution gray crops (the crop rectangle is cropRect()).
	bool refine(Circle &circle, const std::vector<std::vector<uint8_t>> &crops);
	QRect cropRect(const Circle &circle) const;

	struct Spot {
		float x, y;
	};
	std::vector<std::vector<Spot>> findSpots(const std::vector<uint8_t> &median);
	//min_found: fraction of the rays where the outline must be found.
	bool traceOutline(const std::vector<uint8_t> &img, int width, int height, QPointF center, float spread, int nrays, float min_found, Circle &circle);
	bool outline(const std::vector<uint8_t> &median, const std::vector<uint8_t> &bright, int width, int height, QPointF center, float spread, int nrays, float min_found, Circle &circle);
};

#endif // SPHERELOCATOR_H
