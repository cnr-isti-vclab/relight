#include "project.h"
#include "exif.h"
#include "sphere.h"
#include "measure.h"
#include "align.h"
#include "white.h"

#include <QFile>
#include <QTextStream>

#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QPen>
#include <QImageReader>

#include <iostream>

using namespace std;


Project::~Project() {
	clear();
}

void Project::clear() {
	dir = QDir();
	imgsize = QSize();
	images.clear();
	missing.clear();

	for(auto sphere: spheres)
		delete sphere;
	spheres.clear();

	for(auto m: measures)
		delete m;
	measures.clear();

	crop = QRect();
}

bool Project::setDir(QDir folder) {
	if(!folder.exists()) {
		//ask the user for a directory!
		QString folder = QFileDialog::getExistingDirectory(nullptr, "Could not find the image folder: select the images folder.");
		if(folder.isNull()) return false;
	}
	dir = folder;
	QDir::setCurrent(dir.path());
	return true;
}

bool Project::scanDir() {
	QStringList img_ext;
	//TODO add support for raw files (needs preprocessing?)
	img_ext << "*.jpg" << "*.JPG"; // << "*.NEF" << "*.CR2";

	QVector<QSize> resolutions;
	vector<int> count;
	for(QString &s: QDir(dir).entryList(img_ext)) {
		Image image(s);

		QImageReader reader(s);
		image.size = reader.size();

		int index = resolutions.indexOf(image.size);
		if(index == -1) {
			resolutions.push_back(image.size);
			count.push_back(1);
		} else
			count[index]++;
		images.push_back(image);
	}
	if(!images.size())
		return false;

	int max_n = 0;
	for(int i = 0; i < resolutions.size(); i++) {
		if(count[i] > max_n) {
			max_n = count[i];
			imgsize = resolutions[i];
		}
	}


	for(Image &image: images) {
		image.valid = image.size == imgsize;
		image.skip = !image.valid;
	}


	lens.width = imgsize.width();
	lens.height = imgsize.height();

	QVector<Lens> alllens;
	QVector<double> focals;
	count.clear();
	for(Image &image: images) {
		Lens image_lens;
		image_lens.width = lens.width;
		image_lens.height = lens.height;
		try {
			Exif exif;//exif
			exif.parse(image.filename);
#ifdef DEBUG
			for(auto it = exif.begin(); it != exif.end(); it++) {
				cout << qPrintable(exif.tagNames[it.key()]) << " " << qPrintable(it.value().toString()) << endl;
			}
#endif
			image.readExif(exif);
			image_lens.readExif(exif);
		} catch(QString err) {
			//qMessageBox()
			cout << qPrintable(err) << endl;
		}

		alllens.push_back(image_lens);
		int index = focals.indexOf(image_lens.focal35());

		if(index == -1) {
			focals.push_back(image_lens.focal35());
			count.push_back(1);
		} else
			count[index]++;
	}

	max_n = 0;
	for(int i = 0; i < focals.size(); i++) {
		if(count[i] > max_n) {
			max_n = count[i];
			lens = alllens[i];
		}
	}
	for(uint32_t i = 0; i < images.size(); i++) {
		images[i].valid &= (lens.focal35() == alllens[i].focal35());
		images[i].skip = !images[i].valid;
	}

	return resolutions.size() == 1 && focals.size() == 1;
}
double mutualInfo(QImage &a, QImage &b) {
	uint32_t histogram[256*256];
	memset(histogram, 0, 256*256*4);
	for(int y = 0; y < a.height(); y++) {
		const uint8_t *u = a.scanLine(y);
		const uint8_t *v = b.scanLine(y);
		for(int x = 0; x < a.width(); x++) {
			histogram[u[x*3+1] + v[x*3+1]*256]++;
		}
	}
	uint32_t histo1DA[256];
	uint32_t histo1DB[256];
	memset(histo1DA, 0, 256*4);
	memset(histo1DB, 0, 256*4);

	double n = 0.0;

	int i = 0;
	for(int y = 0; y < 256; y++) {
		unsigned int &b = histo1DB[y];
		for(int x = 0; x < 256; x++) {
			double ab = histogram[i++];
			histo1DA[x] += ab;
			b += ab;
		}
		n += b;
	}

	double m = 0.0;
	for(int y = 0; y < 256; y++)
		for(int x = 0; x < 256; x++) {
			double ab = histogram[x + 256*y]/n;
			if(ab == 0) continue;
			double a = histo1DA[x]/n;
			double b = histo1DB[y]/n;
			m += ab * log2((ab)/(a*b));
			m -= a*log(a) + b*log(b);
		}
	//n should be width*height when no border
	return m;
}

void Project::rotateImages(bool clockwise) {
    QTransform rotate;
    rotate.rotate(clockwise ? 90 : -90);
    for(Image &image: images) {
        QImage source(image.filename);
        QImage rotated = source.transformed(rotate);
        rotated.save(image.filename, "jpg", 100);
    }
}

void Project::rotateImages() {
	//find first image non rotated.
	QString target_filename;
	for(Image &image: images) {
		if(image.size == imgsize)
			target_filename = image.filename;
	}

	int width = 300;
	QImage target(target_filename);
	target = target.scaledToWidth(width);
	for(Image &image: images) {
		if(!image.isRotated(imgsize))
			continue;

		QImage source(image.filename);

		QImage thumb = source.scaledToHeight(width);
		QTransform rot_right;
		rot_right.rotate(90);
		QImage right = thumb.transformed(rot_right);

		QTransform rot_left;
		rot_left.rotate(-90);
		QImage left = thumb.transformed(rot_left);

		double right_mutual = mutualInfo(target, right);
		double left_mutual = mutualInfo(target, left);

		QTransform final = right_mutual > left_mutual ? rot_left : rot_right;
		//TODO should be libjpeg to rotate.
		QImage rotated = source.transformed(final);
        rotated.save(image.filename, "jpg", 100);

		image.size = imgsize;
		image.valid = true;
		image.skip = false;
		cout << "Right: " << right_mutual << " Left: " << left_mutual << endl;
	}
}


void Project::load(QString filename) {
	QFile file(filename);
	if(!file.open(QFile::ReadOnly))
		throw QString("Failed opening: " + filename);

	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	QJsonObject obj = doc.object();

	if(obj.contains("version"))
		version = obj["version"].toString();
	if(obj.contains("authors"))
		for(auto author: obj["authors"].toArray())
			authors.push_back(author.toString());
	if(obj.contains("platform"))
		platform = obj["platform"].toString();
	if(obj.contains("created"))
		created = QDateTime::fromString(obj["created"].toString(), Qt::ISODate);

	imgsize.setWidth(obj["width"].toInt());
	imgsize.setHeight(obj["height"].toInt());
	if(!imgsize.isValid())
		throw "Missing or invalid width and/or height.";

	QFileInfo info(filename);
	QDir folder = info.dir();
	folder.cd(obj["folder"].toString());

	if(!setDir(folder))
		throw(QString("Can't load a project without a valid folder"));

	lens.fromJson(obj["lens"].toObject());
	lens.width = imgsize.width();
	lens.height = imgsize.height();

	dome.fromJson(obj["dome"].toObject());

	for(auto img: obj["images"].toArray()) {
		Image image;
		image.fromJson(img.toObject());
		images.push_back(image);
	}

	checkMissingImages();
	checkImages();

	if(obj.contains("crop")) {
		QJsonObject c = obj["crop"].toObject();
		crop.setLeft(c["left"].toInt());
		crop.setTop(c["top"].toInt());
		crop.setWidth(c["width"].toInt());
		crop.setHeight(c["height"].toInt());
	}

	if(obj.contains("spheres")) {
		for(auto sphere: obj["spheres"].toArray()) {
			Sphere *_sphere = new Sphere;
			_sphere->fromJson(sphere.toObject());
			spheres.push_back(_sphere);
		}
	}

	if(obj.contains("measures")) {
		for(auto jmeasure: obj["measures"].toArray()) {
			Measure *measure = new Measure;
			measure->fromJson(jmeasure.toObject());
			measures.push_back(measure);
		}
	}
	if(obj.contains("aligns")) {
		for(auto align: obj["aligns"].toArray()) {
			Align *_align = new Align(0);
			_align->fromJson(align.toObject());
			aligns.push_back(_align);
		}
	}
	if(obj.contains("whites")) {
		for(auto white: obj["whites"].toArray()) {
			White *_white = new White();
			_white->fromJson(white.toObject());
			whites.push_back(_white);
		}
	}
}

void Project::checkMissingImages() {
	missing.clear();
	for(size_t i = 0; i < images.size(); i++) {
		Image &image = images[i];
		QFileInfo imginfo(image.filename);
		if(!imginfo.exists()) {
			missing.push_back(int(i));
			//throw QString("Could not find the image: " + image.filename) + " in folder: " + dir.absolutePath();
			continue;
		}
	}
}
void Project::checkImages() {
		for(Image &image:images) {
		QImageReader reader(image.filename);
		QSize size = reader.size();
		image.valid = (size == imgsize);
		if(!image.valid) image.skip = true;
	}
}


void Project::save(QString filename) {

	QJsonObject project;
	version = RELIGHT_STRINGIFY(RELIGHT_VERSION);

	project.insert("application", "RelightLab");
	project.insert("version", version);
	project.insert("name", name);
	QJsonArray jauthors;
	for(QString &author: authors)
		jauthors.append(author);
	project.insert("authors", jauthors);
	project.insert("platform", platform);
	project.insert("created", created.toString(Qt::ISODate));
	project.insert("lastUpdated", lastUpdated.toString(Qt::ISODate));

	project.insert("width", imgsize.width());
	project.insert("height", imgsize.height());

	//as a folder for images compute the relative path to the saving file location!
	QFileInfo info(filename);
	QString path = info.absoluteDir().relativeFilePath(dir.absolutePath());
	project.insert("folder", path);

	QJsonArray jimages;
	for(auto &img: images)
		jimages.push_back(img.toJson());

	project.insert("images", jimages);


	QJsonObject jlens = lens.toJson();
	project.insert("lens", jlens);

	QJsonObject jdome = dome.toJson();
	project.insert("dome", jdome);

	if(crop.isValid()) {
		QJsonObject jcrop;
		jcrop.insert("left", crop.left());
		jcrop.insert("top", crop.top());
		jcrop.insert("width", crop.width());
		jcrop.insert("height", crop.height());
		project.insert("crop", jcrop);
	}

	QJsonArray jspheres;
	for(auto sphere: spheres)
		jspheres.append(sphere->toJson());
	project.insert("spheres", jspheres);


	QJsonArray jmeasures;
	double length = 0;
	double pixels = 0;
	for(Measure *measure: measures) {
		jmeasures.append(measure->toJson());
		length += measure->length;
		pixels += QLineF(measure->first, measure->second).length();
	}
	project.insert("measures", jmeasures);

	QJsonArray jaligns;
	for(auto align: aligns)
		jaligns.append(align->toJson());
	project.insert("aligns", jaligns);

	QJsonArray jwhites;
	for(auto white: whites)
		jwhites.append(white->toJson());
	project.insert("whitess", jwhites);

	if(length != 0)
		project.insert("scale", length/pixels);

	QJsonDocument doc(project);


	QFile file(filename);
	file.open(QFile::WriteOnly | QFile::Truncate);
	file.write(doc.toJson());
}

Measure *Project::newMeasure() {
	auto m = new Measure();
	measures.push_back(m);
	return m;
}
void Project::computePixelSize() {
	pixelSize = 0;
	float count = 0;
	for(Measure *m: measures)
		if(m->isValid()) {
			pixelSize += m->pixelSize();
			count++;
		}
	pixelSize /= count;
}

Sphere *Project::newSphere() {
	auto s = new Sphere(images.size());
	spheres.push_back(s);
	return s;
}
Align *Project::newAlign() {
	auto s = new Align(images.size());
	aligns.push_back(s);
	return s;
}
White *Project::newWhite() {
	auto s = new White();
	whites.push_back(s);
	return s;
}


void Project::saveLP(QString filename, std::vector<Vector3f> &directions) {
	QFile file(filename);
	if(!file.open(QFile::WriteOnly)) {
		QString error = file.errorString();
		throw error;
	}
	QTextStream stream(&file);

	//computeDirections();

	int invalid_count = 0;
	for(auto d: directions)
		if(d.isZero())
			invalid_count++;

	if(invalid_count)
		QMessageBox::warning(nullptr, "Saving LP :" + filename, QString("Missing %1 light directions").arg(invalid_count));

	stream << directions.size() << "\n";
	for(size_t i = 0; i < directions.size(); i++) {
		Vector3f d = directions[i];
		stream << images[i].filename << " " << d[0] << " " << d[1] << " " << d[2] << "\n";
	}
	QFile obj("sphere.obj");
	obj.open(QFile::WriteOnly);
	QTextStream str(&obj);
	for(size_t i = 0; i < directions.size(); i++) {
		Vector3f d = directions[i];
		str << "v " << d[0] << " " << d[1] << " " << d[2] << "\n";
	}
}

float lineSphereDistance(const Vector3f &origin, const Vector3f &direction, const Vector3f &center, float radius) {
	float a = direction.norm();
	float b = direction*(origin - center)*2.0f;
	float c = center.squaredNorm() + origin.squaredNorm() + center*origin*2.0f - radius*radius;

	float det = b*b - 4.0f*a*c;
	if(det <= 0)
		return 0;
	float d = (-b + sqrt(det))/(2.0f*a);
	return d;
}
void  Project::computeDirections() {
	if(spheres.size() == 0) {
		QMessageBox::critical(nullptr, "Missing light directions.", "Light directions can be loaded from a .lp file or processing the spheres.");
		return;
	}
	vector<Vector3f> directions(size(), Vector3f(0, 0, 0));
	vector<float> weights(size(), 0.0f);
	if(spheres.size()) {
		for(auto sphere: spheres) {
			sphere->computeDirections(lens);
			if(sphere->directions.size() != size())
				throw QString("Sphere number of directions is different than images");

			//if we have a focal length we can rotate the directions of the lights appropriately, unless in the center!
			if(lens.focalLength && (sphere->center != QPointF(0, 0))) {
				//we need to take into account the fact thet the sphere is not centered.
				//we adjust by the angle with the view direction of the highlight.


				float bx = sphere->center.x();
				float by = sphere->center.y();
				Vector3f viewDir = lens.viewDirection(bx, by);
				viewDir.normalize();
				float angle = acos(Vector3f(0, 0, -1) * viewDir);
				//cout << "angle: " << 180*angle/M_PI << endl;
				Vector3f axis = Vector3f(viewDir[1], - viewDir[0], 0);
				axis.normalize();

				for(Vector3f &v: sphere->directions)
					v = v.rotate(axis, angle);

				if(dome.domeDiameter) {
				//find intersection between direAlignctions and sphere.
					for(size_t i = 0; i < sphere->directions.size(); i++) {
						Vector3f &direction = sphere->directions[i];
						direction.normalize();
						Vector3f origin = lens.viewDirection(sphere->lights[i].x(), sphere->lights[i].y());
						//bring it back to surface plane
						origin[2] = 0;
						//normalize by width
						origin /= lens.ccdWidth();

						float radius = (dome.domeDiameter/dome.imageWidth)/2;
						Vector3f center(0, 0, dome.verticalOffset/dome.imageWidth);
						float distance = lineSphereDistance(origin, direction, center, radius);
						Vector3f position = origin + direction*distance;
						images[i].position = position;
						direction = (position - Vector3f(0, 0, dome.verticalOffset/dome.imageWidth))/radius;
					}
				}
			}

			for(size_t i = 0; i < sphere->directions.size(); i++) {
				Vector3f d = sphere->directions[i];
				if(d.isZero())
					continue;
				directions[i] += d;
				weights[i] += 1.0f;
			}
		}
	}

	//Simple mean for the spheres directions (not certainly the smartest thing).
	for(size_t i = 0; i < directions.size(); i++) {
		if(weights[i] > 0)
			images[i].direction = directions[i]/weights[i];
	}
}
