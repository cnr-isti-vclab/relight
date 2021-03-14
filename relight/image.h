#ifndef IMAGE_H
#define IMAGE_H

#include <QString>
#include "../src/vector.h"


class QJsonObject;


class Image {
public:
	QString filename;
	bool valid = true;
	bool skip = false;
	Vector3f direction = Vector3f(0, 0, 0);
	Vector3f position = Vector3f(0, 0, 0);

	Image(QString s = ""): filename(s) {}

	QJsonObject toJson();
	void fromJson(const QJsonObject &obj);
	void readExif();
};

#endif // IMAGE_H
