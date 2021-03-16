#ifndef IMAGE_H
#define IMAGE_H

#include <QString>
#include "../src/vector.h"


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
	uint32_t width = 0, height =0;

	Image(QString s = ""): filename(s) {}

	QJsonObject toJson();
	void fromJson(const QJsonObject &obj);
	void readExif(Exif &exif);
};

#endif // IMAGE_H
