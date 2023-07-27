#ifndef SPHERE_H
#define SPHERE_H

#include <QPoint>
#include <QSize>
#include <QRect>
#include <QImage>
#include <vector>

#include "../src/relight_vector.h"

class QJsonObject;
class Lens;

class Sphere {
public:
	Sphere(int n_lights) {
		lights.resize(n_lights);
		directions.resize(n_lights);
		//valid.resize(n_lights, false);
	}


	QPointF center;      //in pixel coordinates of the image
	float radius;        //fitted radius
	float smallradius;   //innner radius where to look for reflections
	QRect inner;         //box of the inner part of the sphere.
	bool fitted;         //we have a valid fit
	QImage sphereImg;

	std::vector<QPointF> border;
	std::vector<QPointF> lights;       //2d pixel of the light spot for this sphere.
	std::vector<Vector3f> directions;  //

	std::vector<std::vector<int>>histogram;

	Sphere();
	bool fit();
	void ellipseFit();
	void findHighlight(QImage im, int n);
	void computeDirections(Lens &lens);

	void resetHighlight(size_t n); //reset light and direction of the detected highlight, of image n.

	QJsonObject toJson();
	void fromJson(QJsonObject obj);
};

#endif // SPHERE_H
