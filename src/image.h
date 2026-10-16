#ifndef IMAGE_H
#define IMAGE_H

#include <QString>
#include <QSize>

#include <Eigen/Core>


class QJsonObject;
class Exif;

class Image {
public:
	QString filename;
	bool valid = true;
	bool skip = false;  //not counted for lps or spheres
	bool visible = true;  //counted with lp but still not used for rti.

	double exposureTime = 0;
	double isoSpeedRatings = 0;
	QString colorSpace;
	QSize size;

	Image(QString s = ""): filename(s) {}

	QJsonObject toJson();
	void fromJson(const QJsonObject &obj);
	void readExif(Exif &exif);
	bool isRotated(QSize &imgsize) {
		if(size.width() == imgsize.width())  //square images are problematic :P
			return false;

		return size == QSize(imgsize.height(), imgsize.width());
	}
};

#endif // IMAGE_H
