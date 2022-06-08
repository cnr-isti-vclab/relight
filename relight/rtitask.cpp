#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QSettings>
#include <QRect>
#include <QTemporaryDir>

#include "rtitask.h"
#include "zoom.h"
#include "../src/rti.h"
#include "../src/deepzoom.h"
#include "../relight-cli/rtibuilder.h"


#include <iostream>
using namespace std;

int convertToRTI(const char *filename, const char *output);
int convertRTI(const char *file, const char *output, int quality);

RtiTask::RtiTask() {}

RtiTask::~RtiTask() {
	if(builder)
		delete builder;
}


void relight();
void toRTI();
void fromRTI();
void itarzoom();


void RtiTask::run() {
	status = RUNNING;
	QStringList steps = (*this)["steps"].value.toStringList();
    std::function<bool(std::string s, int d)> callback = [this](std::string s, int n)->bool { return this->progressed(s, n); };
    QString err;
	for(auto step: steps) {
		if(step == "relight")
			relight();
		else if(step == "rti")
			relight(true, true);
		else if(step == "fromRTI")
			fromRTI();
		//TODO! deepZOOM should set error and status?
        else if(step == "deepzoom") {
			if ((err = deepZoom(input_folder, output, 95, 0, 256, callback)).compare("OK") != 0) {
				error = err;
                status = FAILED;
			}
        }
        else if(step == "tarzoom") {
			if ((err = tarZoom(output, output, callback)).compare("OK") != 0) {
				error = err;
                status = FAILED;
			}
        }
        else if(step == "itarzoom") {
			if ((err = itarZoom(output, output, callback)).compare("OK") != 0) {
				error = err;
                status = FAILED;
			}
        }
		else if(step == "openlime")
			openlime();
	}
	if(status != FAILED)
		status = DONE;
}

void  RtiTask::relight(bool commonMinMax, bool saveLegacy) {
	builder = new RtiBuilder;
	builder->pixelSize =(*this)["pixelSize"].value.toDouble();
	builder->commonMinMax = commonMinMax;

	builder->nworkers = QSettings().value("nworkers", 8).toInt();
	builder->samplingram = QSettings().value("ram", 512).toInt();

	builder->samplingram = (*this)["ram"].value.toInt();
	builder->type         = Rti::Type((*this)["type"].value.toInt());
	builder->colorspace   = Rti::ColorSpace((*this)["colorspace"].value.toInt());
	builder->nplanes      = (*this)["nplanes"].value.toInt();
	builder->yccplanes[0] = (*this)["yplanes"].value.toInt();
	//builder->sigma =

	if( builder->colorspace == Rti::MYCC) {
		builder->yccplanes[1] = builder->yccplanes[2] = (builder->nplanes - builder->yccplanes[0])/2;
		builder->nplanes = builder->yccplanes[0] + 2*builder->yccplanes[1];
	}

	builder->imageset.images = (*this)["images"].value.toStringList();
	QList<QVariant> qlights = (*this)["lights"].value.toList();
	std::vector<Vector3f> lights(qlights.size()/3);
	for(int i = 0; i < qlights.size(); i+= 3)
		for(int k = 0; k < 3; k++)
			lights[i/3][k] = qlights[i+k].toDouble();
	builder->lights = builder->imageset.lights = lights;
	builder->imageset.initImages(input_folder.toStdString().c_str());

	if(hasParameter("crop")) {
		QRect rect = (*this)["crop"].value.toRect();
		builder->crop[0] = rect.left();
		builder->crop[1] = rect.top();
		builder->crop[2] = rect.width();
		builder->crop[3] = rect.height();
		builder->imageset.crop(rect.left(), rect.top(), rect.width(), rect.height());
	}
	builder->width  = builder->imageset.width;
	builder->height = builder->imageset.height;
	int quality= (*this)["quality"].value.toInt();

	std::function<bool(std::string s, int n)> callback = [this](std::string s, int n)->bool { return this->progressed(s, n); };

	try {
		if(!builder->init(&callback)) {
			error = builder->error.c_str();
			status = FAILED;
			return;
		}
		if(saveLegacy) {
			if(builder->type == Rti::HSH)
				builder->saveUniversal(output.toStdString());
		} else
			builder->save(output.toStdString(), quality);

	} catch(std::string e) {
		error = e.c_str();
		status = STOPPED;
		return;
	}
}

void RtiTask::toRTI() {
	QString filename = output;
	QTemporaryDir tmp;
	if(!tmp.isValid()) {
		cerr << "OOOPSS" << endl;
		return;
	}
	output = tmp.path();
	relight(true);
	try {
		convertToRTI(tmp.filePath("info.json").toLatin1().data(), filename.toLatin1().data());
	} catch(QString err) {
		error = err;
		status = FAILED;
	}
}

void RtiTask::fromRTI() {
	QString input = (*this)["input"].value.toString();
	int quality= (*this)["quality"].value.toInt();
	try {
		convertRTI(input.toLatin1().data(), output.toLatin1().data(), quality);
	} catch(QString err) {
		error = err;
		status = FAILED;
	}
}

int nPlanes(QString output) {
	QDir destination(output);
	return destination.entryList(QStringList("plane_*.jpg"), QDir::Files).size();
}

void RtiTask::openlime() {
	QStringList files = QStringList() << ":/demo/index.html"
									  << ":/demo/openlime.min.js"
									  << ":/demo/skin.css"
									  << ":/demo/skin.svg";
	QDir dir(output);
	for(QString file: files) {
		QFile fp(file);
		fp.open(QFile::ReadOnly);
		QFileInfo info(file);
		QFile copy(dir.filePath(info.fileName()));
		copy.open(QFile::WriteOnly);
		copy.write(fp.readAll());
	}
}

bool RtiTask::progressed(std::string s, int percent) {
	QString str(s.c_str());
	emit progress(str, percent);
	if(status == PAUSED) {
		mutex.lock();  //mutex should be already locked. this talls the
		mutex.unlock();
	}
	if(status == STOPPED)
		return false;
	return true;
}
