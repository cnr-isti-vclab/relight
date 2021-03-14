#ifndef PROJECT_H
#define PROJECT_H

#include "measure.h"
#include "ball.h"
#include "lens.h"
#include "image.h"
#include <QDir>
#include <QString>

class QJsonObject;


class Project {
public:

	QDir dir;                  //image folder
	QSize imgsize;             //images width and height (must be the same for all).
	Lens lens;


	std::vector<Image> images1;
	std::map<int, Ball *> balls;
	std::vector<Measure *> measures;
	QRect crop;

	Project() {}
	~Project();

	void clear();
	void load(QString filename);
	void save(QString filename);
	void saveLP(QString filename, std::vector<Vector3f> &directions);
	void computeDirections();

	bool setDir(QDir folder);
	bool scanDir(); //load images from project.dir, and return false if some problems with resolution.
	bool hasDirections() {
		for(auto &im: images1)
			if(!im.direction.isZero())
				return true;
		return false;
	}
	size_t size() { return images1.size(); }
	QStringList images() {
		QStringList imgs;
		for(Image &img: images1)
			imgs.push_back(img.filename);
		return imgs;
	}
	std::vector<Vector3f> directions() {
		std::vector<Vector3f> dirs;
		for(Image &img: images1)
			dirs.push_back(img.direction);
		return dirs;
	}
	int indexOf(QString s) {
		for(size_t i = 0; i < images1.size(); i++)
			if(images1[i].filename == s)
				return i;
		return -1;
	}


};

#endif // PROJECT_H
