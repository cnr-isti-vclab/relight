#ifndef CROP_H
#define CROP_H

#include <QRect>
#include <QPolygon>
#include <QTransform>
#include <vector>
#include <Eigen/Core>

/* Crop coordinates:
 *
 * The crop coordinates are relative to the rotated image bounding box
 *
 */

class Crop: public QRect {
public:
	float angle = 0.0f;
	bool operator==(const Crop &crop) {
		return angle == crop.angle && rect() == crop.rect();
	}
	void operator=(const QRect &rect) {
		setRect(rect);
	}
	const QRect &rect() const {
		return *(QRect *)this;
	}
	void setRect(const QRect &rect) {
		QRect::setRect(rect.left(), rect.top(), rect.width(), rect.height());
	}
	void setRect(const QPoint &p, const QSize &s) {
		QRect::setRect(p.x(), p.y(), s.width(), s.height());
	}
	//convert a point in image coords (top left is 0,0) to the rotated image bounding box coords
	QPointF imgToCrop(QPointF p, QSize img_size);
	//viceversa
	QPointF cropToImg(QPointF p, QSize img_size);
	//bounding box of the cropped region (including rotation)
	QRect boundingRect(QSize img_size) const;
	QImage cropBoundingImage(QImage src);
	std::vector<float> cropBoundingNormals(const std::vector<float> &input, int &w, int &h);
	std::vector<Eigen::Vector3f> cropBoundingNormals(const std::vector<Eigen::Vector3f> &input, int &w, int &h);
};

#endif // CROP_H
