#include "project.h"

#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

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
	dir = info.dir();
	dir.cd(obj["folder"].toString());

	if(!dir.exists()) {
		//ask the user for a directory!
		QString folder = QFileDialog::getExistingDirectory(nullptr, "Could not find the image folder: select the images folder.");
		if(folder.isNull()) return;
		dir = QDir(folder);
	}

	images.clear();
	for(auto img: obj["images"].toArray()) {
		QString filename = img.toString();
		QFileInfo imginfo(filename);
		if(!imginfo.exists())
			throw QString("Could not find the image: " + filename + " in folder: " + dir.absolutePath());

		images.push_back(filename);
	}

	directions.clear();
	if(obj.contains("directions")) {
		for(auto direction: obj["directions"].toArray()) {
			auto d = direction.toArray();
			directions.push_back(Vector3f(d[0].toDouble(), d[1].toDouble(), d[2].toDouble()));
		}
	}

	positions.clear();
	if(obj.contains("positions")) {
		for(auto position: obj["positions"].toArray()) {
			auto d = position.toArray();
			positions.push_back(Vector3f(d[0].toDouble(), d[1].toDouble(), d[2].toDouble()));
		}
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
			Ball ball;
			ball.fromJsonObject(sphere.toObject());
			balls[count++] = ball;
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

	QJsonArray jimages = QJsonArray::fromStringList(images);
	project.insert("images", jimages);

	if(directions.size()) {
		QJsonArray jdirections;
		for(Vector3f &d: directions) {
			QJsonArray jd = { d[0], d[1], d[2] };
			jdirections.push_back(jd);
		}
		project.insert("directions", jdirections);
	}

	if(positions.size()) {
		QJsonArray jpositions;
		for(Vector3f &d: positions) {
			QJsonArray jd = { d[0], d[1], d[2] };
			jpositions.push_back(jd);
		}
		project.insert("positions", jpositions);
	}

	if(crop.isValid()) {
		QJsonObject jcrop;
		jcrop.insert("left", crop.left());
		jcrop.insert("top", crop.top());
		jcrop.insert("width", crop.width());
		jcrop.insert("height", crop.height());
		project.insert("crop", jcrop);
	}

	QJsonArray spheres;
	for(auto ball: balls)
		spheres.append(ball.second.toJsonObject());

	QJsonDocument doc(project);
	doc.toJson();
}
