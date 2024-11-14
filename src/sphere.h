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
class Dome;

struct Line {
	Vector3f origin;
	Vector3f direction;
};

class Sphere {
public:
	//TODO: initalize these values somewhere.
	QPointF center;      //center of circle or ellipse, in pixel coordinates of the image

	/* Circle parameters */
	float radius;        //fitted radius
	float smallradius;   //innner radius where to look for reflections

	/* Ellipse parameters */
	bool ellipse = false;
	float eWidth, eHeight, eAngle;
	float eFocal; //estimated focal

	QRect inner;         //box of the inner part of the circle/ellipse
	bool fitted;         //we have a valid fit
	QImage sphereImg;
	std::vector<QImage> thumbs;


	std::vector<QPointF> border;        //2d pixels sampled on the border of the sphere.
	std::vector<QPointF> lights;       //2d pixel of the light spot for this sphere.
	std::vector<Vector3f> directions;  //


	Sphere(int n_lights = 0);

	bool fit();
	void ellipseFit();
	void findHighlight(QImage im, int n, bool update_positions = true);

	//compute lights directions relative to the center of the sphere.
	void computeDirections(Lens &lens);
	Line toLine(Vector3f dir, Lens &lens);
	static Vector3f intersection(std::vector<Line> &lines);

	void resetHighlight(size_t n); //reset light and direction of the detected highlight, of image n.

	QJsonObject toJson();
	void fromJson(QJsonObject obj);
};

//estimate light directions relative to the center of the image.
void computeDirections(std::vector<Sphere *> &spheres, Lens &lens, std::vector<Vector3f> &directions);
//estimate light positions using parallax (image width is the unit).
void computeParallaxPositions(std::vector<Sphere *> &spheres, Lens &lens, std::vector<Vector3f> &positions);
//estimate light positions assuming they live on a sphere (parameters provided by dome
void computeSphericalPositions(std::vector<Sphere *> &spheres, Dome &dome, Lens &lens, std::vector<Vector3f> &positions);

#endif // SPHERE_H
