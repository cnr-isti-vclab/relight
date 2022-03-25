#include <vips/vips.h>

#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QSettings>
#include <QRect>

#include "rtitask.h"
#include "zoom.h"
#include "../src/rti.h"
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
		else if(step == "toRTI")
			toRTI();
		else if(step == "fromRTI")
			fromRTI();
        else if(step == "deepzoom") {
            if ((err = deepZoom(input_folder, output, 95, 0, 256, callback)).compare("OK") != 0)
                status = FAILED;
        }
        else if(step == "tarzoom") {
            if ((err = tarZoom(output, output, callback)).compare("OK") != 0)
                status = FAILED;
        }
		else if(step == "itarzoom")
			itarzoom();
		else if(step == "openlime")
			openlime();
	}
	if(status != FAILED)
		status = DONE;
    else
        error = err;
    qDebug() << "zoom error: " << err;
}

void  RtiTask::relight() {
	builder = new RtiBuilder;
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

		builder->save(output.toStdString(), quality);

	} catch(std::string e) {
		error = e.c_str();
		status = STOPPED;
		return;
	}
}

void RtiTask::toRTI() {
	convertToRTI((output + ".rti").toLatin1().data(), output.toLatin1().data());
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

void RtiTask::itarzoom() {
	int nplanes = nPlanes(output);
	QStringList args;
	for(int i = 0; i < nplanes; i++)
		args << QString("plane_%1.tzi").arg(i);
	args << "planes";
	runPythonScript("itarzoom.py", args, output);
	cout << qPrintable(log) << endl;
	if(status == FAILED)
		return;

	progressed("Itarzoom:", 100);

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

void RtiTask::pause() {
	mutex.lock();
	status = PAUSED;
}

void RtiTask::resume() {
	if(status == PAUSED) {
		status = RUNNING;
		mutex.unlock();
	}
}

void RtiTask::stop() {
	if(status == PAUSED) { //we were already locked then.
		status = STOPPED;
		mutex.unlock();
	}
	status = STOPPED;
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
