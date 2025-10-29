#include "dome.h"
#include "image.h"
#include "sphere.h"
#include "lens.h"
#include "lp.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QList>
#include <QFile>

#include <Eigen/Dense>

using namespace std;
using namespace Eigen;

static float lineSphereDistance(const Vector3f &origin, const Vector3f &direction, const Vector3f &center, float radius) {
	float a = direction.norm();
	float b = direction.dot(origin - center)*2.0f;
	float c = center.squaredNorm() + origin.squaredNorm() + center.dot(origin)*2.0f - radius*radius;

	float det = b*b - 4.0f*a*c;
	if(det <= 0)
		return 0;
	float d = (-b + sqrt(det))/(2.0f*a);
	return d;
}

Dome::Dome() {}

static QStringList lightConfigs = { "directional", "spherical", "lights3d" };


void Dome::parseLP(const QString &lp_path) {
	std::vector<QString> filenames;
	::parseLP(lp_path, directions, filenames);
	lightConfiguration = DIRECTIONAL;
}

QJsonArray toJson(std::vector<Vector3f> &values) {
	QJsonArray jvalues;
	for(Vector3f v: values) {
		QJsonObject obj;
		obj["x"] = v[0];
		obj["y"] = v[1];
		obj["z"] = v[2];
		jvalues.append(obj);
	}
	return jvalues;
}

QJsonArray toJson(std::vector<Color3f> &values) {
	QJsonArray jvalues;
	for(Color3f v: values) {
		QJsonObject obj;
		obj["r"] = v[0];
		obj["g"] = v[1];
		obj["b"] = v[2];
		jvalues.append(obj);
	}
	return jvalues;
}

//rename in recompute directions.
void Dome::fromSpheres(std::vector<Image> &images, std::vector<Sphere *> &spheres, Lens &lens) {

	if(spheres.size() == 0) {
		throw QString("Light directions can be loaded from a .lp file or processing the spheres.");
	}
	//count valid images:
	size_t valid_count = 0;
	for(Image &img: images)
		if(!img.skip)
			valid_count++;

	directions.clear();
	directions.resize(valid_count, Vector3f(0, 0, 0));
	positions3d.clear();
	positions3d.resize(valid_count, Vector3f(0, 0, 0));
	positionsSphere.clear();
	positionsSphere.resize(valid_count, Vector3f(0, 0, 0));

	vector<float> weights(valid_count, 0.0f);

	for(auto sphere: spheres) {
		sphere->computeDirections(lens);

		if(sphere->directions.size() != images.size()) //directions for skipped images are still there (0, though)
			throw QString("Sphere number of directions is different than images");

		size_t count = 0;
		//if we have a focal length we can rotate the directions of the lights appropriately, unless in the center!
		for(size_t i = 0; i < sphere->directions.size(); i++) {
			if(images[i].skip)
				continue;
			Vector3f d = sphere->directions[i];
			if(!d.isZero()) {
				directions[count] += d;
				weights[count] += 1.0f; //actually weight should be the distance of the light to the center.
			}
			count++;
		}
		assert(count == directions.size());
	}


	for(size_t i = 0; i < directions.size(); i++) {
		if(weights[i] == 0)
			continue;
		directions[i] /= weights[i];
	}

	if(domeDiameter && imageWidth) {
		for(auto sphere: spheres) {
			//find intersection between reflection directions and sphere.
			//we are working in normalized coordinates where imageWidth is 1.0
			size_t count = 0;
			for(size_t i = 0; i < sphere->directions.size(); i++) {
				if(images[i].skip)
					continue;

				Vector3f direction = sphere->directions[i];
				if(!direction.isZero()) {

					direction.normalize();
					//this is in focal ccd width coords.
					Vector3f origin = lens.viewDirection(sphere->lights[i].x(), sphere->lights[i].y());
					//bring it back to surface plane
					origin[2] = 0;
					//normalize by width
					origin /= lens.ccdWidth();

					float radius = (domeDiameter/2.0f)/imageWidth;
					//Here a small error could hide, because of the z of the sphere is not on the plane of the object.
					Vector3f center(0, 0, verticalOffset/imageWidth);
					float distance = lineSphereDistance(origin, direction, center, radius);
					Vector3f position = origin + direction*distance;
					direction = (position - Vector3f(0, 0, verticalOffset/imageWidth))/radius;
					positionsSphere[count] += position*imageWidth;
				}
				count++;
			}
			assert(count == directions.size());
		}
		for(size_t i = 0; i < positionsSphere.size(); i++) {
			if(weights[i] == 0)
				continue;
			positionsSphere[i] /= weights[i];
		}
	}

	if(spheres.size() > 1) {
		computeParallaxPositions(images, spheres, lens, positions3d);
		float factor = imageWidth ? imageWidth*2.0f : 1.0f;
		for(Vector3f &p: positions3d)
			p *= factor;
		assert(positions3d.size() == directions.size());
	}
}

void Dome::save(const QString &filename) {
	QFile file(filename);
	bool open = file.open(QFile::WriteOnly | QFile::Truncate);
	if(!open) {
		throw file.errorString();
	}
	QJsonDocument doc(toJson());
	file.write(doc.toJson());
}

void Dome::load(const QString &filename) {
	QFile file(filename);
	bool open = file.open(QFile::ReadOnly);
	if(!open) {
		throw file.errorString();
	}
	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	fromJson(doc.object());
}


QJsonObject Dome::toJson() {
	QJsonObject dome;
	dome.insert("label", label);
	dome.insert("notes", notes);
	dome.insert("imageWidth", imageWidth);
	dome.insert("domeDiameter", domeDiameter);
	dome.insert("verticalOffset", verticalOffset);
	dome.insert("lightConfiguration", lightConfigs[lightConfiguration]);
	dome.insert("directions", ::toJson(directions));
	dome.insert("positionsSphere", ::toJson(positionsSphere));
	dome.insert("positions3d", ::toJson(positions3d));
	dome.insert("ledAdjust", ::toJson(ledAdjust));
	return dome;
}

 void fromJson(QJsonArray values, std::vector<Vector3f> &lights) {
	lights.clear();
	for(auto v: values) {
		auto o = v.toObject();
		lights.push_back(Vector3f(o["x"].toDouble(), o["y"].toDouble(), o["z"].toDouble()));
	}
}

void fromJson(QJsonArray values, std::vector<Color3f> &lights) {
	lights.clear();
	for(auto v: values) {
		auto o = v.toObject();
		lights.push_back(Color3f(o["r"].toDouble(), o["g"].toDouble(), o["b"].toDouble()));
	}
}


void Dome::fromJson(const QJsonObject &obj) {
	label = obj["label"].toString();
	notes = obj["notes"].toString();
	imageWidth     = obj["imageWidth"].toDouble();
	domeDiameter   = obj["domeDiameter"].toDouble();
	verticalOffset = obj["verticalOffset"].toDouble();
	if(obj.contains("lightConfiguration")) {
		lightConfiguration = DIRECTIONAL;
		int index = lightConfigs.indexOf(obj["lightConfiguration"].toString());
		if(index >= 0)
			lightConfiguration = LightConfiguration(index);
	}
	::fromJson(obj["directions"].toArray(), directions);
	::fromJson(obj["positionsSphere"].toArray(), positionsSphere);
	::fromJson(obj["positions3d"].toArray(), positions3d);
	::fromJson(obj["ledAdjust"].toArray(), ledAdjust);
}

