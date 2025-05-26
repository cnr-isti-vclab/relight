#ifndef CROP_H
#define CROP_H

#include <QRect>

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
};

#endif // CROP_H
