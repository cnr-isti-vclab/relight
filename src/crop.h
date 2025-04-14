#ifndef CROP_H
#define CROP_H

#include <QRect>

class Crop: public QRect {
public:
	void setRect(QRect r) {
		static_cast<QRect&>(*this) = r;
	}
	float angle = 0.0f;

};

#endif // CROP_H
