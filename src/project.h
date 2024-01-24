#ifndef PROJECT_H
#define PROJECT_H

#define RELIGHT_STRINGIFY0(v) #v
#define RELIGHT_STRINGIFY(v) RELIGHT_STRINGIFY0(v)

#include "lens.h"
#include "dome.h"
#include "image.h"
#include <QDir>
#include <QString>
#include <QRect>
#include <QSysInfo>
#include <QDateTime>

class QJsonObject;
class Sphere;
class Measure;
class Align;
class White;


class Project {
public:

	QString version;
	QDir dir;                  //image folder, path relative to project
	QSize imgsize;             //images width and height (must be the same for all).
	Lens lens;
	Dome dome;

	std::vector<Image> images;
	std::vector<int> missing; //missing images on load
	std::vector<Sphere *> spheres;
	std::vector<Measure *> measures;
	std::vector<Align *> aligns;
	std::vector<White *> whites;
	QRect crop;
	float pixelSize = 0; //if computed from measures

	QString name;
	std::vector<QString> authors;
	QString platform;
	QDateTime created;
	QDateTime lastUpdated;

	Project() {
		version = RELIGHT_STRINGIFY(RELIGHT_VERSION);
		auto sysinfo = QSysInfo();
		platform = sysinfo.kernelType() + " " + sysinfo.kernelVersion();
		created = lastUpdated = QDateTime::currentDateTime();
	}
	~Project();

	void clear();
	void load(QString filename);
	void save(QString filename);
	void saveLP(QString filename, std::vector<Vector3f> &directions);
	void computeDirections();
	void computePixelSize();

	Measure *newMeasure();
	Sphere *newSphere();
	Align *newAlign();
	White *newWhite();

	//set image folder
	bool setDir(QDir folder);
	bool scanDir(); //load images from project.dir, and return false if some problems with resolution.
	void rotateImages();
    void rotateImages(bool clockwise);
	bool hasDirections() {
		for(auto &im: images)
			if(!im.direction.isZero())
				return true;
		return false;
	}
	size_t size() { return images.size(); }

	QStringList getImages() {
		QStringList imgs;
		for(Image &img: images)
			if(!img.skip)
				imgs.push_back(img.filename);
		return imgs;
	}
	//check size and validity
	void checkImages();
	void checkMissingImages();

	std::vector<Vector3f> directions() {
		std::vector<Vector3f> dirs;
		for(Image &img: images)
			if(!img.skip)
				dirs.push_back(img.direction);
		return dirs;
	}
	int indexOf(QString s) {
		for(size_t i = 0; i < images.size(); i++)
			if(images[i].filename == s)
				return i;
		return -1;
	}
	int indexOf(Sphere *sphere) {
		for(size_t i = 0; i < spheres.size(); i++) {
			if(sphere == spheres[i])
				return i;
		}
		return -1;
	}

};

#endif // PROJECT_H
