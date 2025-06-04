#ifndef CROP_H
#define CROP_H

#include <QRect>
#include <QPolygon>
#include <QTransform>
#include <vector>

/* Crop coordinates:
 *
 * The crop coordinates are relative to the rotated image bounding box?
 *
 */

class Crop: public QRect {
public:
	QRect rect;
	float angle = 0.0f;
	bool operator==(const Crop &crop) {
		return angle == crop.angle && rect == crop.rect;
	}

	//bounding box of the cropped region (including rotation)
	QRect boundingRect(QSize img_size);
	QImage cropBoundingImage(QImage src);
	std::vector<float> cropBoundingNormals(const std::vector<float> &input, int &w, int &h);
};

#endif // CROP_H
