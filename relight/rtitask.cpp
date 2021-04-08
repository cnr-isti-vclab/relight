#include <QFileInfo>
#include <QFile>
#include <QDir>

#include "rtitask.h"
#include "../src/rti.h"
#include "../relight-cli/rtibuilder.h"

RtiTask::RtiTask()
{

}

RtiTask::~RtiTask() {
	if(builder)
		delete builder;
}


void RtiTask::run() {
	status = RUNNING;
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

	std::function<bool(std::string s, int n)> callback = [this](std::string s, int n)->bool { return this->progressed(s, n); };

	try {
		if(!builder->init(&callback)) {
			error = builder->error.c_str();
			status = FAILED;
			return;
		}
		int quality= (*this)["quality"].value.toInt();
		builder->save(output.toStdString(), quality);
	} catch(std::string e) {
		error = e.c_str();
		status = STOPPED;
		return;
	}
	if((*this)["openlime"].value.toBool()) {
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

	status = DONE;
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
