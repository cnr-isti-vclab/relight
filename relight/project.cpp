#include "project.h"

#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>
#include <QImageReader>
#include <QMessageBox>

using namespace std;

QJsonObject Image::save() {

	QJsonObject obj;
	obj.insert("filename", filename);
	obj.insert("skip", skip);

	QJsonArray dir { direction[0], direction[1], direction[2] };
	obj.insert("direction", dir);

	QJsonArray pos { position[0], position[1], position[2] };
	obj.insert("position", pos);

	return obj;
}
void Image::load(QJsonObject obj) {
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

	for(auto img: obj["images"].toArray()) {
		Image image;
		image.load(img.toObject());

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
			ball->fromJsonObject(sphere.toObject());
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

	QJsonArray jimages;
	for(auto &img: images1)
		jimages.push_back(img.save());

	project.insert("images", jimages);

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
		spheres.append(ball.second->toJsonObject());

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
