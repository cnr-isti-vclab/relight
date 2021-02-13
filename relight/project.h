#ifndef PROJECT_H
#define PROJECT_H

#include <QDir>
#include <QString>
#include "ball.h"

class Project {
public:

	Project() {}

	void load(QString filename);
	void save(QString filename);


	QDir dir;                  //image folder
	QSize imgsize;             //images width and height (must be the same for all).
	QStringList images;
	std::vector<bool> valid;   //valid images.

	std::map<int, Ball> balls;

	std::vector<Vector3f> directions;  //light directions as computed from the balls.
	std::vector<Vector3f> positions;   //3d positions if using more than 1 ball.

	QRect crop;
};

#endif // PROJECT_H
