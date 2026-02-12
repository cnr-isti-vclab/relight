#ifndef PROJECT_H
#define PROJECT_H

#define RELIGHT_STRINGIFY0(v) #v
#define RELIGHT_STRINGIFY(v) RELIGHT_STRINGIFY0(v)

#include "lens.h"
#include "dome.h"
#include "image.h"
#include "crop.h"
#include <QDir>
#include <QString>
#include <QRect>
#include <QSysInfo>
#include <QDateTime>
#include <QList>
#include <QJsonObject>

class QJsonObject;
class Sphere;
class Measure;
class Align;
class White;

class Project {
public:

	QString version;
	QDir dir;                  //image folder, path relative to project
	//TODO: this is duplicated in lens!
	QSize imgsize;             //images width and height (must be the same for all).
	Lens lens;
	Dome dome;

	std::vector<Image> images;
	std::vector<int> missing; //missing images on load
	std::vector<Sphere *> spheres;
	std::vector<Measure *> measures;
	std::vector<Align *> aligns;
	std::vector<White *> whites;
	Crop crop;
	std::vector<QPointF> offsets;
	float pixelSize = 0.0f; //if computed from measures in mm

	// ICC color profile information (detected from images)
	QString icc_profile_description;
	bool icc_profile_is_srgb = false;
	bool icc_profile_is_display_p3 = false;

	QString name;
	std::vector<QString> authors;
	QString platform;
	QDateTime created;
	QDateTime lastUpdated;
	bool needs_saving;
	QList<QJsonObject> taskHistoryEntries;

	Project() {
		version = RELIGHT_STRINGIFY(RELIGHT_VERSION);
		auto sysinfo = QSysInfo();
		platform = sysinfo.kernelType() + " " + sysinfo.kernelVersion();
		created = lastUpdated = QDateTime::currentDateTime();
		needs_saving = false;
	}
	~Project();

	void clear();
	void load(QString filename);
	void save(QString filename);
	void saveLP(QString filename, std::vector<Eigen::Vector3f> &directions);
	void cleanAlignCache();
	void cleanSphereCache();
	void addCompletedTask(const QJsonObject &info);
	void removeTaskFromHistory(const QString &historyId);
	const QList<QJsonObject> &taskHistory() const { return taskHistoryEntries; }
	void clearTaskHistory();

	void computeOffsets(); //from aligns
	void computeDirections(); //obsolete
	void computePixelSize();

	Measure *newMeasure();
	Sphere *newSphere();
	Align *newAlign();
	White *newWhite();

	//set image folder
	bool setDir(QDir folder);
	bool scanDir(); //load images from project.dir, and return false if some problems with resolution.
	QImage readImage(int i);
	bool rotateImage(Image &image, bool clockwise);
	void rotateImages();
	void rotateImages(bool clockwise);
	bool hasDirections() { return dome.directions.size() > 0; }
	//return number of non skipped images.
	size_t size() { 
		size_t count = 0;
		for(const Image &img: images)
			if(!img.skip)
				count++;
		return count;
	}

	//remove skipped
	QStringList getImages() const {
		QStringList imgs;
		for(Image img: images)
			if(!img.skip)
				imgs.push_back(img.filename);
		return imgs;
	}
	//check size and validity
	void checkImages();
	void checkMissingImages();
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
	void validateDome(size_t n_lights); //throw an error in case the number of lights is not consistent.
};

#endif // PROJECT_H
