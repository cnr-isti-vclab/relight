#ifndef IMAGE_H
#define IMAGE_H

#include <QString>
#include <QSize>
#include "../src/relight_vector.h"


class QJsonObject;
class Exif;

class Image {
public:
	QString filename;
	bool valid = true;
	bool skip = false;
	Vector3f direction = Vector3f(0, 0, 0);  //light direction (infinite distance assumed)
	Vector3f position = Vector3f(0, 0, 0);   //3d light position
	double exposureTime = 0;
	double isoSpeedRatings = 0;
	QSize size;

	Image(QString s = ""): filename(s) {}

	QJsonObject toJson();
	bool hasLightDirection() {
		return !direction.isZero() || !position.isZero();
	}
	void fromJson(const QJsonObject &obj);
	void readExif(Exif &exif);
	/*bool matchSize(QSize &imgsize) {
		size == imgsize;
		return int(width) == imgsize.width() && int(height) == imgsize.height();
	}*/
	bool isRotated(QSize &imgsize) {
		if(size.width() == imgsize.width())  //square images are problematic :P
			return false;

		return size == QSize(imgsize.height(), imgsize.width());
	}
};

#endif // IMAGE_H
