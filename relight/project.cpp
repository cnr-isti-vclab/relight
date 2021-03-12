#include "project.h"

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

#include "../src/qexifimageheader.h"
using namespace std;

QJsonObject Image::toJson() {

	QJsonObject obj;
	obj.insert("filename", filename);
	obj.insert("skip", skip);

	QJsonArray dir { direction[0], direction[1], direction[2] };
	obj.insert("direction", dir);

	QJsonArray pos { position[0], position[1], position[2] };
	obj.insert("position", pos);

	return obj;
}
void Image::fromJson(const QJsonObject &obj) {
	filename = obj["filename"].toString();
	skip = obj["skip"].toBool();

	QJsonArray dir = obj["directions"].toArray();
	direction[0] = dir[0].toDouble();
	direction[1] = dir[1].toDouble();
	direction[2] = dir[2].toDouble();

	QJsonArray pos = obj["positions"].toArray();
	position[0] = pos[0].toDouble();
	position[1] = pos[1].toDouble();
	position[2] = pos[2].toDouble();
}


Project::~Project() {
	clear();
}

void Project::clear() {
	dir = QDir();
	imgsize = QSize();
	images1.clear();

	for(auto b: balls)
		delete b.second;
	balls.clear();

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
	img_ext << "*.jpg" << "*.JPG" << "*.NEF" << "*.CR2";

	QVector<QSize> resolutions;
	vector<int> count;
	for(QString &s: QDir(dir).entryList(img_ext)) {
		images1.push_back(Image(s));
		QImageReader reader(s);
		QSize size = reader.size();
		if(resolutions.indexOf(size)== -1) {
			resolutions.push_back(size);
			count.push_back(1);
		}
	}
	if(!images1.size())
		return false;
	int max_n = 0;
	for(int i = 0; i < resolutions.size(); i++) {
		if(count[i] > max_n) {
			max_n = count[i];
			imgsize = resolutions[i];
		}
	}

	for(Image &image: images1) {
		QImageReader reader(image.filename);

		QSize size = reader.size();
		image.valid = (size != imgsize);

		QExifImageHeader exif;
		exif.loadFromJpeg(image.filename);
		auto tags = exif.imageTags();
		cout << exif.contains(QExifImageHeader::ImageWidth) << ": " << exif.value(QExifImageHeader::ImageWidth).toLong() << endl;
		//ImageWidth ImageLength

		auto extended = exif.extendedTags();
		cout << exif.value(QExifImageHeader::ExposureTime).type() << " " << qPrintable(exif.value(QExifImageHeader::ExposureTime).toRational().second) << endl;
		//ColorSpace
		//PixelXDimension
		//PixelYDimension
		//FocalLength
		//FocalLengthIn35mmFilm
		//FocalPlaneXResolution
		//FocalPlaneYResolution
		//FocalPlaneResolutionUnit
		//ExposureTime
		//FNumber
		//ShutterSpeedValue
		//FocalLength
	}
	return resolutions.size() == 1;
}

void Project::load(QString filename) {
	QFile file(filename);
	if(!file.open(QFile::ReadOnly))
		throw QString("Failed opening: " + filename);

	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	QJsonObject obj = doc.object();
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

	for(auto img: obj["images"].toArray()) {
		Image image;
		image.fromJson(img.toObject());

		QFileInfo imginfo(image.filename);
		if(!imginfo.exists())
			throw QString("Could not find the image: " + image.filename) + " in folder: " + dir.absolutePath();

		QImageReader reader(image.filename);
		QSize size = reader.size();
		image.valid = (size != imgsize);

		images1.push_back(image);
	}

	if(obj.contains("crop")) {
		QJsonObject c = obj["crop"].toObject();
		crop.setLeft(c["left"].toInt());
		crop.setTop(c["top"].toInt());
		crop.setWidth(c["width"].toInt());
		crop.setHeight(c["height"].toInt());
	}

	if(obj.contains("spheres")) {
		int count =0 ;
		for(auto sphere: obj["spheres"].toArray()) {
			Ball *ball = new Ball;
			ball->fromJson(sphere.toObject());
			balls[count++] = ball;
		}
	}

	if(obj.contains("measures")) {
		for(auto jmeasure: obj["measures"].toArray()) {
			Measure *measure = new Measure;
			measure->fromJson(jmeasure.toObject());
			measures.push_back(measure);
		}
	}
}

void Project::save(QString filename) {

	QJsonObject project;
	project.insert("width", imgsize.width());
	project.insert("height", imgsize.height());

	//as a folder for images compute the relative path to the saving file location!
	QFileInfo info(filename);
	QString path = dir.relativeFilePath(info.absoluteDir().absolutePath());
	project.insert("folder", path);

	QJsonArray jimages;
	for(auto &img: images1)
		jimages.push_back(img.toJson());

	project.insert("images", jimages);


	QJsonObject jlens = lens.toJson();
	project.insert("lens", jlens);

	if(crop.isValid()) {
		QJsonObject jcrop;
		jcrop.insert("left", crop.left());
		jcrop.insert("top", crop.top());
		jcrop.insert("width", crop.width());
		jcrop.insert("height", crop.height());
		project.insert("crop", jcrop);
	}

	QJsonArray jspheres;
	for(auto it: balls)
		jspheres.append(it.second->toJson());
	project.insert("spheres", jspheres);


	QJsonArray jmeasures;
	for(Measure *measure: measures)
		jmeasures.append(measure->toJson());
	project.insert("measures", jmeasures);

	QJsonDocument doc(project);


	QFile file(filename);
	file.open(QFile::WriteOnly | QFile::Truncate);
	file.write(doc.toJson());
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
		QMessageBox::warning(nullptr, "Saving LP :" + filename, QString("Saving LP will skip %1 missing light directions"));

	stream << directions.size() << "\n";
	for(size_t i = 0; i < directions.size(); i++) {
		Vector3f d = directions[i];
		if(d.isZero())
			continue;
		stream << images1[i].filename << " " << d[0] << " " << d[1] << " " << d[2] << "\n";
	}
}

void  Project::computeDirections() {
	if(balls.size() == 0) {
		QMessageBox::critical(nullptr, "Missing light directions.", "Light directions can be loaded from a .lp file or processing the spheres.");
		return;
	}
	vector<Vector3f> directions(size(), Vector3f(0, 0, 0));
	vector<float> weights(size(), 0.0f);
	if(balls.size()) {
		for(auto it: balls) {
			Ball *ball = it.second;
			ball->computeDirections();
			if(ball->directions.size() != size())
				throw QString("Ball number of directions is different than images");

			for(size_t i = 0; i < ball->directions.size(); i++) {
				Vector3f d = ball->directions[i];
				if(d.isZero())
					continue;
				directions[i] += d;
				weights[i] += 1.0f;
			}
		}
	}

	//Simple mean for the balls directions (not certainly the smartest thing).
	for(size_t i = 0; i < directions.size(); i++) {
		if(weights[i] > 0)
			images1[i].direction = directions[i]/weights[i];
	}

}
