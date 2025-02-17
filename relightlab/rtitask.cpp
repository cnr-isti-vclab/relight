#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QSettings>
#include <QRect>
#include <QTemporaryDir>

#include "rtitask.h"
#include "../relight/zoom.h"
#include "../src/rti.h"
#include "../relight-cli/rtibuilder.h"


#include <iostream>
using namespace std;

int convertToRTI(const char *filename, const char *output);
int convertRTI(const char *file, const char *output, int quality);

void setupLights(ImageSet &imageset, Dome &dome);

QString RtiParameters::summary() {
	QString basisLabels[] =  { "PTM", "HSH", "RBF", "BLN", "NEURAL" };
	QString colorspaceLabels[] =  { "RGB", "LRGB", "YCC", "RGB", "YCC" };
	QString formatLabels[] = { "", "web: images", "web: deepzoom", "web: tarzoom", "web: itarzoom", "IIIF: tiff" };

	QString s_basis  = basisLabels[basis];
	QString s_colorspace = colorspaceLabels[colorspace];
	QString s_planes = QString::number(nplanes);
	if(nchroma) {
		s_planes += "." + QString::number(nchroma);
	}
	QString s_format;
	if(format == RtiParameters::RTI)
		s_format = basis == Rti::PTM ? ".ptm" : ".rti";
	else
		s_format = formatLabels[format];

	QString txt = QString("%1%3 (%2) %4").arg(s_basis).arg(s_colorspace).arg(s_planes).arg(s_format);
	return txt;
}

RtiTask::RtiTask(): Task() {
	builder = new RtiBuilder;
}

void RtiTask::setProject(Project &project) {
	builder->imageset.pixel_size = project.pixelSize;

	builder->nworkers = QSettings().value("nworkers", 8).toInt();
	builder->samplingram = QSettings().value("ram", 512).toInt();



	ImageSet &imageset = builder->imageset;

	imageset.images = project.getImages();
	imageset.initImages(project.dir.absolutePath().toStdString().c_str());

	imageset.initFromDome(project.dome); //lights after images
	imageset.setCrop(project.crop, project.offsets);
	imageset.pixel_size = project.pixelSize;
	builder->sigma = 0.125*100/imageset.images.size();
	cout << "builder->sigma: " << builder->sigma << endl;

	builder->crop[0] = imageset.left;
	builder->crop[1] = imageset.top;
	builder->crop[2] = imageset.width;
	builder->crop[3] = imageset.height;

	builder->width  = imageset.width;
	builder->height = imageset.height;
}

RtiTask::~RtiTask() {
	if(builder)
		delete builder;
}

void RtiTask::setParameters(RtiParameters &p) {
	parameters = p;
	label = parameters.summary();

	builder->type         = parameters.basis;
	builder->colorspace   = parameters.colorspace;
	builder->nplanes      = parameters.nplanes;
	builder->yccplanes[1] = builder->yccplanes[2] = parameters.nchroma;

	if( builder->colorspace == Rti::MYCC) {
		if((parameters.nchroma + parameters.nplanes)%2 == 1) {
			builder->nplanes += 3;
		}
		builder->yccplanes[0] = (builder->nplanes - 2*builder->yccplanes[1]);
		//builder->yccplanes[1] = builder->yccplanes[2] = (builder->nplanes - builder->yccplanes[0])/2;
		//builder->nplanes = builder->yccplanes[0] + 2*builder->yccplanes[1];
	}

	//legacy format uses the same scale and bias for each component (RBG)
	if(parameters.format == RtiParameters::RTI)
		builder->commonMinMax = true;

}

void RtiTask::run() {

	status = RUNNING;	
	std::function<bool(QString s, int d)> callback = [this](QString s, int n)->bool { return this->progressed(s, n); };

	QString output = parameters.path; //masking Task::output.
	try {
		if(!builder->init(&callback)) {
			error = builder->error.c_str();
			status = FAILED;
			return;
		}
		if(parameters.format == RtiParameters::RTI) {
			if(builder->type == Rti::HSH) {
				mime = RTI;
				builder->saveUniversal(output.toStdString());
			} else if(builder->type == Rti::PTM) {
				mime = PTM;
				builder->savePTM(output.toStdString());
			} else
				throw "Legacy RTI and PTM formats are supported only for HSH and PTM basis";
		} else {
			mime = RELIGHT;
			builder->save(output.toStdString(), parameters.quality);
		}

		if(parameters.openlime && parameters.format == RtiParameters::WEB)
			openlime();

		if(parameters.web_layout != RtiParameters::PLAIN) {
			deepZoom(output, output, parameters.quality, 0, 256, callback);
		}
		if(parameters.web_layout == RtiParameters::TARZOOM) {
			tarZoom(output, output, callback);
		}
		if(parameters.web_layout == RtiParameters::ITARZOOM) {
			itarZoom(output, output, callback);
		}


	} catch(std::string e) {
		error = e.c_str();
		status = STOPPED;
		return;
	}

	if(status != FAILED)
		status = DONE;
}
/*
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

	imageset.images = (*this)["images"].value.toStringList();
	QList<QVariant> qlights = (*this)["lights"].value.toList();
	std::vector<Vector3f> lights(qlights.size()/3);
	for(int i = 0; i < qlights.size(); i+= 3)
		for(int k = 0; k < 3; k++)
			lights[i/3][k] = qlights[i+k].toDouble();
	builder->lights = imageset.lights = lights;
	imageset.light3d = project.dome.lightConfiguration != Dome::DIRECTIONAL;
	imageset.dome_radius = project.dome.domeDiameter/2.0;
	imageset.vertical_offset = project.dome.verticalOffset;
	imageset.initLights();
	imageset.initImages(input_folder.toStdString().c_str());


	if(hasParameter("crop")) {
		QRect rect = (*this)["crop"].value.toRect();
		builder->crop[0] = rect.left();
		builder->crop[1] = rect.top();
		builder->crop[2] = rect.width();
		builder->crop[3] = rect.height();
		imageset.crop(rect.left(), rect.top(), rect.width(), rect.height());
	}
	builder->width  = imageset.width;
	builder->height = imageset.height;
	int quality= (*this)["quality"].value.toInt();

	std::function<bool(QString s, int n)> callback = [this](QString s, int n)->bool { return this->progressed(s, n); };

	try {
		if(!builder->init(&callback)) {
			error = builder->error.c_str();
			status = FAILED;
			return;
		}
		if(saveLegacy) {
			if(builder->type == Rti::HSH)
				builder->saveUniversal(output.toStdString());
			else if(builder->type == Rti::PTM)
				builder->savePTM(output.toStdString());
			else
				throw "Legacy RTI and PTM formats are supported only for HSH and PTM basis";
		} else
			builder->save(output.toStdString(), quality);

	} catch(std::string e) {
		error = e.c_str();
		status = STOPPED;
		return;
	}
}
*/

/* not used anymore: build temporary rti and convert to legacy format */
/* void RtiTask::toRTI() {
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
} */

/*
void RtiTask::fromRTI() {
	QString input = (*this)["input"].value.toString();
	int quality= (*this)["quality"].value.toInt();
	try {
		convertRTI(input.toLatin1().data(), output.toLatin1().data(), quality);
	} catch(QString err) {
		error = err;
		status = FAILED;
	}
} */


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
