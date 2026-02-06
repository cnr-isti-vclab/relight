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
#include <QFileInfo>
#include <Eigen/Dense>

using namespace std;
using namespace Eigen;

//estimate light positions using parallax (image width is the unit).
void computeParallaxPositions(std::vector<Image> &images, std::vector<Sphere *> &spheres, Lens &lens, std::vector<Eigen::Vector3f> &positions);

//compute the distance of the
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

Eigen::Vector3f findIntersection(const Eigen::Vector3f& origin,
								 const Eigen::Vector3f& direction,
								 const Eigen::Vector3f& center,
								 double radius) {
	Eigen::Vector3f L = origin - center;
	Eigen::Vector3f d = direction.normalized();

	float b = 2.0 * L.dot(d);
	float c = L.squaredNorm() - (radius * radius);
	float discriminant = b * b - 4.0 * c;

	float t = (-b + std::sqrt(discriminant)) / 2.0;
	return origin + t * d;
}

Dome::Dome() {}

static QStringList lightConfigs = { "directional", "spherical", "lights3d" };
static QStringList lightSources = { "unknown", "from_spheres", "from_lp" };


void Dome::parseLP(const QString &lp_path) {
	std::vector<QString> filenames;
	::parseLP(lp_path, directions, filenames);

	QFileInfo info(lp_path);
	label = info.filePath();
	lightSource = FROM_LP;
	recomputePositions();
}

void Dome::recomputePositions() {
	// Recompute 3D positions from directions when geometry parameters change	
	positionsSphere.clear();
	positions3d.clear();
	
	if(!domeDiameter || !imageWidth || directions.empty())
		return;
	
	positionsSphere.resize(directions.size(), Vector3f(0, 0, 0));
	positions3d.resize(directions.size(), Vector3f(0, 0, 0));
	
	// Compute sphere positions from directions
	for(size_t i = 0; i < directions.size(); i++) {
		Vector3f direction = directions[i];
		if(direction.isZero())
			continue;
			
		direction.normalize();
		
		float radius = (domeDiameter/2.0f);
		positions3d[i] = positionsSphere[i] = direction*radius + Eigen::Vector3f(0, 0, verticalOffset);
	}
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

	directions.clear();
	positions3d.clear();
	positionsSphere.clear();

	//count valid images:
	size_t valid_count = 0;
	for(Image &img: images)
		if(!img.skip)
			valid_count++;


	directions.resize(valid_count, Vector3f(0, 0, 0));
	positions3d.resize(valid_count, Vector3f(0, 0, 0));
	positionsSphere.resize(valid_count, Vector3f(0, 0, 0));

	if(spheres.size() == 0) {
		//		throw QString("Light directions can be loaded from a .lp file or processing the spheres.");
		return;
	}

	vector<float> weights(valid_count, 0.0f);

	lightSource = FROM_SPHERES;

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
			if(sphere->reflections.size() == 0)
				sphere->computeDirections(lens);
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
					//even worse: the starting point should the the reflection point.

					Vector3f center(0, 0, verticalOffset/imageWidth);
					Vector3f reflection = sphere->reflections[i] - Vector3f(lens.width/2.0f, lens.height/2.0f, 0);
					reflection /= lens.width;

					Vector3f position = findIntersection(origin, direction, center, radius);
					//float distance = lineSphereDistance(reflection, direction, center, radius);
					//Vector3f position = origin + direction*distance;
					//direction = (position - Vector3f(0, 0, verticalOffset/imageWidth))/radius;
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


//find the intersection of the lines using least squares approximation.
Vector3f intersection(std::vector<Line> &lines) {
	using Mat3 = Eigen::Matrix3d;
	using Vec3 = Eigen::Vector3d;

	Mat3 M = Mat3::Zero();
	Vec3 b = Vec3::Zero();

	for (const auto &L : lines) {
		Vec3 u(L.direction[0], L.direction[1], L.direction[2]);
		double n = u.norm();
		if (n < 1e-12) continue;       // skip degenerate directions
		u /= n;

		Vec3 o(L.origin[0], L.origin[1], L.origin[2]);
		Mat3 P = Mat3::Identity() - u * u.transpose(); // projects onto plane ⟂ to u

		M += P;
		b += P * o;
	}

	// Tikhonov regularization for near-parallel sets
	const double lambda = 1e-9;
	M += lambda * Mat3::Identity();

	Vec3 x = M.ldlt().solve(b);
	return Vector3f(float(x[0]), float(x[1]), float(x[2]));
}

//move this stuff in Dome.
//estimate light positions using parallax (image width is the unit).
void computeParallaxPositions(std::vector<Image> &images, std::vector<Sphere *> &spheres, Lens &lens, std::vector<Vector3f> &positions) {
	positions.clear();

	if(spheres.size() < 2)
		return;

	for(Sphere *sphere: spheres)
		sphere->computeDirections(lens);


	//for each reflection, compute the lines and the best intersection, estimate the radiuus of the positions vertices
	vector<float> radia;
	for(size_t i = 0; i < spheres[0]->directions.size(); i++) {
		if(images[i].skip)
			continue;

		std::vector<Line> lines;
		for(Sphere *sphere: spheres) {
			if(sphere->directions[i].isZero()) continue;
			lines.push_back(sphere->toLine(sphere->directions[i], lens));
		}
		Vector3f position = intersection(lines);

		radia.push_back(position.norm());
		positions.push_back(position);
	}
	//find median
	size_t m = radia.size()/2;
	std::nth_element(radia.begin(), radia.begin() + m, radia.end());
	float radius  = radia[m];
	//if some directions is too different from the average radius, we bring the direction closer to the average.
	float threshold = 0.1;
	for(Vector3f &dir: positions) {
		float d = dir.norm();
		if(fabs(d - radius) > threshold*radius) {
			dir *= radius/d;
		}
		if(dir[2] < 0) //might be flipped for nearly parallel directions.
			dir *= -1;
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
	QFileInfo info(filename);
	if(info.suffix().toLower() == "lp") {
		parseLP(filename);
		return;
	}

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
	dome.insert("lightSource", lightSources[lightSource]);
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
	if(obj.contains("lightSource")) {
		lightSource = UNKNOWN;
		int index = lightSources.indexOf(obj["lightSource"].toString());
		if(index >= 0)
			lightSource = LightSource(index);
	} else {
		lightSource = FROM_LP;
	}
	::fromJson(obj["directions"].toArray(), directions);
	::fromJson(obj["positionsSphere"].toArray(), positionsSphere);
	::fromJson(obj["positions3d"].toArray(), positions3d);
	::fromJson(obj["ledAdjust"].toArray(), ledAdjust);
}

