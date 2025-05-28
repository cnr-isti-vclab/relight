#ifndef CROP_H
#define CROP_H

#include <QRect>
#include <QPolygon>
#include <QTransform>

#include <Eigen/Core>
#include <vector>

/* Crop coordinates:
 *
 * The crop coordinates are relative to the rotated image bounding box.
 *
 */

class Crop: public QRect {
public:
	void setRect(QRect r) {
		static_cast<QRect&>(*this) = r;
	}
	float angle = 0.0f;
	bool operator==(const Crop &crop) {
		return angle == crop.angle && QRect(*this) == QRect(crop);
	}
	//bounding box of the cropped region (including rotation)
	QRect boundingRect(QSize img_size);
	QImage cropBoundingImage(QImage src, QSize img_size);
	std::vector<Eigen::Vector3f> cropBoundingNormals(std::vector<Eigen::Vector3f>, int w, int h, QSize img_size);
	std::vector<Eigen::Vector3f> rotateAndCropImage(
		const std::vector<Eigen::Vector3f>& input,
		int width, int height,
		float angleDeg,
		QRect cropRect // crop rect in rotated image space
	);
};

#endif // CROP_H
