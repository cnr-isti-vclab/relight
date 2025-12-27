#include "project.h"
#include "exif.h"
#include "sphere.h"
#include "measure.h"
#include "align.h"
#include "white.h"
#include "lp.h"
#include "colorprofile.h"
#include "jpeg_decoder.h"

#include <QFile>
#include <QTextStream>

#include <QFileInfo>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>

#include <QPen>
#include <QImageReader>

#include <Eigen/Dense>

#include <iostream>

using namespace std;
using namespace Eigen;

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

	for(auto align: aligns)
		delete align;
	aligns.clear();

	for(auto m: measures)
		delete m;
	measures.clear();

	crop = Crop();
	needs_saving = false;
	icc_profile_description = "No profile";
	icc_profile_is_srgb = false;
	icc_profile_is_display_p3 = false;
	taskHistoryEntries.clear();
}

bool Project::setDir(QDir folder) {
	if(!folder.exists()) {
		return false;
	}
	dir = folder;
	QDir::setCurrent(dir.path());
	needs_saving = true;
	return true;
}

bool Project::scanDir() {
	QStringList img_ext;
	//TODO add support for raw files (needs preprocessing?)
	img_ext << "*.jpg" << "*.JPG" << "*.jpeg" << "*.JPEG"; // << "*.NEF" << "*.CR2";

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
	crop.setRect(QPoint(0, 0), imgsize);

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
	
	// Detect ICC color profile from first valid image
	icc_profile_description = "No profile";
	icc_profile_is_srgb = false;
	icc_profile_is_display_p3 = false;
	
	for(const Image &image: images) {
		if(!image.skip) {
			QString filepath = dir.filePath(image.filename);
			JpegDecoder dec;
			int w, h;
			if(dec.init(filepath.toStdString().c_str(), w, h)) {
				if(dec.hasICCProfile()) {
					bool is_rgb = false;
					std::vector<uint8_t> profile_data = dec.getICCProfile();
					icc_profile_description = ColorProfile::getProfileDescription(profile_data, is_rgb);
					icc_profile_is_srgb = ColorProfile::isSRGBProfile(profile_data);
					icc_profile_is_display_p3 = ColorProfile::isDisplayP3Profile(profile_data);
				}
				break; // Only check first valid image
			}
		}
	}
	
	needs_saving = true;
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
	needs_saving = true;
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
	}
}


void Project::load(QString filename) {
	QFile file(filename);
	if(!file.open(QFile::ReadOnly))
		throw QString("The project file could not be opened.\nThe path might be wrong, or the file missing.");


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
		throw QString("Missing or invalid width and/or height in project.");
	if(obj.contains("pixelSize"))
		pixelSize = obj["pixelSize"].toDouble();

	// Load ICC profile information
	if(obj.contains("iccProfileDescription"))
		icc_profile_description = obj["iccProfileDescription"].toString();
	else
		icc_profile_description = "No profile";
		
	if(obj.contains("iccProfileIsSRGB"))
		icc_profile_is_srgb = obj["iccProfileIsSRGB"].toBool();
	else
		icc_profile_is_srgb = false;

	if(obj.contains("iccProfileIsDisplayP3"))
		icc_profile_is_display_p3 = obj["iccProfileIsDisplayP3"].toBool();
	else
		icc_profile_is_display_p3 = false;

	QFileInfo info(filename);
	QDir folder = info.dir();
	folder.cd(obj["folder"].toString());

	if(!setDir(folder))
		throw QString("The folder " + obj["folder"].toString() + " does not exists.");

	//ensure resources folder has been created (for old project)
	QDir resources = dir.filePath("resources");
	if (!resources.exists() && !dir.mkpath("resources")) {
		throw QString("Could not create the resources folder in %1").arg(dir.absolutePath());
	}

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
		if(c.contains("angle"))
			crop.angle = c["angle"].toDouble();
	}

	if(obj.contains("spheres")) {
		for(auto sphere: obj["spheres"].toArray()) {
			Sphere *_sphere = new Sphere;
			_sphere->fromJson(sphere.toObject());
			_sphere->image_size = imgsize;
			spheres.push_back(_sphere);
		}
	}

	// Backward compatibility: if spheres exist but lightSource wasn't saved (old projects),
	// set lightSource to FROM_SPHERES since that was the implicit behavior
	if(spheres.size() > 0 && dome.lightSource == Dome::UNKNOWN) {
		dome.lightSource = Dome::FROM_SPHERES;
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
		computeOffsets();
	}
	if(obj.contains("whites")) {
		for(auto white: obj["whites"].toArray()) {
			White *_white = new White();
			_white->fromJson(white.toObject());
			whites.push_back(_white);
		}
	}

	taskHistoryEntries.clear();
	if(obj.contains("taskHistory")) {
		for(const auto &entry: obj["taskHistory"].toArray()) {
			taskHistoryEntries.append(entry.toObject());
		}
	}
	needs_saving = false;
}

void Project::checkMissingImages() {
	missing.clear();
	for(size_t i = 0; i < images.size(); i++) {
		Image &image = images[i];
		QFileInfo imginfo(dir.filePath(image.filename));
		if(!imginfo.exists()) {
			missing.push_back(int(i));
		}
	}
}
void Project::checkImages() {
	for(Image &image:images) {
		QImageReader reader(image.filename);
		if(!reader.canRead())
			continue;
		QSize size = reader.size();
		image.valid = (size == imgsize);
		if(!image.valid) image.skip = true;
	}
}


void Project::save(QString filename) {

	QFile file(filename);
	bool opened  = file.open(QFile::WriteOnly | QFile::Truncate);
	if(!opened) {
		QString error = file.errorString();
		throw error;
	}

	lastUpdated = QDateTime::currentDateTime();

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
	project.insert("pixelSize", pixelSize);

	// Save ICC profile information
	if(icc_profile_description != "No profile") {
		project.insert("iccProfileDescription", icc_profile_description);
		project.insert("iccProfileIsSRGB", icc_profile_is_srgb);
		project.insert("iccProfileIsDisplayP3", icc_profile_is_display_p3);
	}

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
		jcrop.insert("angle", crop.angle);
		project.insert("crop", jcrop);
	}

	//ensure resources folder has been created
	QDir resources = dir.filePath("resources");
	if (!resources.exists() && !dir.mkpath("resources")) {
		throw QString("Could not create the resources folder in %1").arg(dir.absolutePath());
	}
	//remove all sphere images.
	QStringList sphereSummary = resources.entryList(QStringList() << "sphere_*x*+*+*.jpg");
	for(QString file: sphereSummary) {
		QFile::remove(resources.filePath(file));
	}

	QJsonArray jspheres;
	for(auto sphere: spheres) {
		if(!sphere->sphereImg.isNull()) {
			QString filename = QString("sphere_%1x%2+%3+%4.jpg")
					.arg(sphere->inner.width())
					.arg(sphere->inner.height())
					.arg(sphere->inner.left())
					.arg(sphere->inner.top());
			sphere->sphereImg.save(resources.filePath(filename));
		}
		jspheres.append(sphere->toJson());
	}
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

	QJsonArray jhistory;
	for(const QJsonObject &entry: taskHistoryEntries)
		jhistory.append(entry);
	project.insert("taskHistory", jhistory);

	if(length != 0)
		project.insert("scale", length/pixels);

	QJsonDocument doc(project);



	file.write(doc.toJson());

	needs_saving = false;
}

void Project::cleanAlignCache() {
	QStringList filenames;
	for(Align *align: aligns) {
		QRect inner = align->rect;
		QString filename = QString("align_%1x%2+%3+%4.jpg")
				.arg(inner.width())
				.arg(inner.height())
				.arg(inner.left())
				.arg(inner.top());
		filenames.push_back(filename);
	}
	QDir dir("./");
	if(!dir.exists("resources"))
		dir.mkdir("resources");
	dir.cd("resources");

	QStringList filters;
	filters << "align_*";
	QStringList files = dir.entryList(filters, QDir::Files);
	for (const QString &file : files) {
		if(!filenames.contains(file))
			QFile::remove(dir.filePath(file));
	}
}

void Project::cleanSphereCache() {
	QStringList filenames;
	for(Sphere *sphere: spheres) {
		QRect &inner = sphere->inner;
		QString filename = QString("spherecache_%1x%2+%3+%4.jpg")
				.arg(inner.width())
				.arg(inner.height())
				.arg(inner.left())
				.arg(inner.top());
		filenames.push_back(filename);
	}

	QDir dir("./");
	if(!dir.exists("resources"))
		dir.mkdir("resources");
	dir.cd("resources");

	QStringList filters;
	filters << "spherecache_*";
	QStringList files = dir.entryList(filters, QDir::Files);
	for (const QString &file : files) {
		if(!filenames.contains(file))
			QFile::remove(dir.filePath(file));
	}
}

void Project::addCompletedTask(const QJsonObject &info) {
	taskHistoryEntries.push_front(info);
	needs_saving = true;
}

void Project::removeTaskFromHistory(const QString &uuid) {
	for(int i = 0; i < taskHistoryEntries.size(); ++i) {
		const QJsonObject &entry = taskHistoryEntries[i];
		const QString entryUuid = entry.value("uuid").toString();
		const bool matchesUuid = !entryUuid.isEmpty() && entryUuid == uuid;
		if(matchesUuid) {
			taskHistoryEntries.removeAt(i);
			needs_saving = true;
			return;
		}
	}
}

void Project::clearTaskHistory() {
	if(taskHistoryEntries.isEmpty())
		return;
	taskHistoryEntries.clear();
	needs_saving = true;
}

Measure *Project::newMeasure() {
	auto m = new Measure();
	needs_saving = true;
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

	if(count == 0)
		return;

	pixelSize /= count;
	needs_saving = true;
	dome.imageWidth = pixelSize*imgsize.width();
}

Sphere *Project::newSphere() {
	auto s = new Sphere(images.size());
	spheres.push_back(s);
	needs_saving = true;
	return s;
}

Align *Project::newAlign() {
	auto s = new Align(images.size());
	aligns.push_back(s);
	needs_saving = true;
	return s;
}

White *Project::newWhite() {
	auto s = new White();
	whites.push_back(s);
	needs_saving = true;
	return s;
}


void Project::saveLP(QString filename, vector<Vector3f> &directions) {
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
		throw QString("Missing %1 light directions").arg(invalid_count);

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

//used only in relight legacy app.
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
void Project::computeOffsets() {
	offsets.clear();
	offsets.resize(images.size(), QPointF(0,0));
	std::vector<float> weights(offsets.size(), 0);
	for(Align *align: aligns) {
		for(size_t i = 0; i < align->offsets.size(); i++) {
			offsets[i] += align->offsets[i];
			weights[i] += 1.0f;
		}
	}
	for(size_t i = 0; i < offsets.size(); i++) {
		if(weights[i] > 0)
			offsets[i] /= weights[i];
	}
}


/* This is obsolete, used only in legacy relight app */
void  Project::computeDirections() {
	if(spheres.size() == 0) {
		throw QString("Light directions can be loaded from a .lp file or processing the spheres.");
	}
	dome.directions.clear();
	dome.directions.resize(size(), Vector3f(0, 0, 0));
	dome.positions3d.clear();
	dome.positions3d.resize(size(), Vector3f(0, 0, 0));
	dome.positionsSphere.clear();
	dome.positionsSphere.resize(size(), Vector3f(0, 0, 0));

	vector<float> weights(size(), 0.0f);

	if(spheres.size() && lens.focalLength) {
		for(auto sphere: spheres) {
			sphere->computeDirections(lens);
			if(sphere->directions.size() != size())
				throw QString("Sphere number of directions is different than images");

			//TODO: this needs to be verified properly.

			//if we have a focal length we can rotate the directions of the lights appropriately, unless in the center!
			if(lens.focalLength && (sphere->center != QPointF(0, 0))) {
				//we need to take into account the fact thet the sphere is not centered.
				//we adjust by the angle with the view direction of the highlight.


				float bx = sphere->center.x();
				float by = sphere->center.y();
				Vector3f viewDir = lens.viewDirection(bx, by);
				viewDir.normalize();
				float angle = acos(Vector3f(0, 0, -1).dot(viewDir));

				Vector3f axis = Vector3f(viewDir[1], - viewDir[0], 0);
				axis.normalize();

				AngleAxisf rotation(angle, axis);
				for(Vector3f &v: sphere->directions) {
					//TODO remove this after verification.
					Vector3f g = v*cos(angle) + axis.cross(v)*sin(angle) + axis *(axis.dot(v)) * (1 - cos(angle));
					v = rotation * v;
					assert(fabs(v[0] - g[0]) < 0.01);
					assert(fabs(v[1] - g[1]) < 0.01);
					assert(fabs(v[2] - g[2]) < 0.01);
				}

				if(dome.domeDiameter) {
					//find intersection between reflection directions and sphere.
					for(size_t i = 0; i < sphere->directions.size(); i++) {
						Vector3f direction = sphere->directions[i];
						if(direction == Vector3f(0, 0, 0))
							continue;

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
						direction = (position - Vector3f(0, 0, dome.verticalOffset/dome.imageWidth))/radius;
						dome.positionsSphere[i] += position;
					}
				}
			}

			for(size_t i = 0; i < sphere->directions.size(); i++) {
				Vector3f d = sphere->directions[i];
				if(d.isZero())
					continue;
				dome.directions[i] += d;
				weights[i] += 1.0f;
			}
		}
	}

	//Simple mean for the spheres directions (if we have a dome diameter, directions are corrected for sphere position.
	for(size_t i = 0; i < dome.directions.size(); i++) {
		if(weights[i] == 0)
			continue;
		dome.directions[i] /= weights[i];
		dome.positions3d[i] /= weights[i];
		dome.positionsSphere[i] /= weights[i];
	}
	needs_saving = true;
}

void Project::validateDome(size_t n_lights) {
	if(size() != n_lights) {
		throw QString("The folder contains %1 images, the .lp file specify %2 images.\n"
					  "You might have some extraneous images, or just loading the wrong .lp file.")
				.arg(size()).arg(n_lights);
	}
}
