#ifndef PROJECT_H
#define PROJECT_H

#include <QDir>
#include <QString>
#include <QJsonObject>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include "ball.h"


class Image {
public:
	Image(QString s = ""): filename(s) {}

	QJsonObject save();
	void load(QJsonObject obj);
	QString filename;
	bool valid = true;
	bool skip = false;
	Vector3f direction = Vector3f(0, 0, 0);
	Vector3f position = Vector3f(0, 0, 0);
};

class Measure {
public:
	Measure() {}
	~Measure();
	
	QPainterPath path();
	QGraphicsPathItem *newPoint(QPointF p);
	void setFirstPoint(QPointF p);
	void setSecondPoint(QPointF p);
	void clear();
	
	QPointF first;
	QPointF second;
	
	QGraphicsPathItem *first_point = nullptr;
	QGraphicsPathItem *second_point = nullptr;
	//QString path = "m -1,1 -4,4 m 6,-4 4,4 m -4,-6 3,-4 m -9,0 4,4";	
	enum Unit { MM, CM, M, INCH, YARD };
	Unit unit;
};

class Project {
public:

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

	QDir dir;                  //image folder
	QSize imgsize;             //images width and height (must be the same for all).
	std::vector<Image> images1;
	std::map<int, Ball *> balls;
	std::vector<Measure *> measures;
	QRect crop;
};

#endif // PROJECT_H
