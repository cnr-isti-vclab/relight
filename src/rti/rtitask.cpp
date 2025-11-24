#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QSettings>
#include <QRect>
#include <QTemporaryDir>
#include <QMessageBox>

#include "rtitask.h"
#include "relightapp.h"
#include "zoom.h"
#include "../src/rti.h"
#include "../src/cli/rtibuilder.h"
#include "../src/deepzoom.h"


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

	builder->nworkers = qRelightApp->nThreads();
	builder->samplingram = qRelightApp->samplingRam();

	crop = project.crop;

	ImageSet &imageset = builder->imageset;
	imageset.initFromProject(project);

	imageset.setCrop(crop, project.offsets);
	imageset.rotateLights(-crop.angle);
	imageset.pixel_size = project.pixelSize;
	builder->sigma = 0.125*100/imageset.images.size();

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
				throw QString("Legacy RTI and PTM formats are supported only for HSH and PTM basis");
		} else {
			mime = RELIGHT;
			builder->save(output.toStdString(), parameters.quality);
		}
		if(crop.angle != 0.0f) {
			rotatedCrop(output);
		}

		if(parameters.openlime && parameters.format == RtiParameters::WEB)
			openlime();

		if(parameters.format == RtiParameters::IIP) {
			// Build TIFF pyramids for IIP using tiffZoom helper
			tiffZoom(output, output, parameters.quality, 256,
				[this](QString s, int n)->bool {
					return progressed(s, n);
				});
		}
		//format is now WEB
		if(parameters.web_layout != RtiParameters::PLAIN) {
			deepZoom(output, output, parameters.quality, 0, 256, callback);
		}
		if(parameters.web_layout == RtiParameters::TARZOOM || parameters.web_layout == RtiParameters::ITARZOOM) {
			tarZoom(output, output, callback);
		}
		if(parameters.web_layout == RtiParameters::ITARZOOM) {
			itarZoom(output, output, callback);
		}
	} catch(QString e) {
		error = e;
		status = FAILED;
		return;
	}

	if(status != FAILED)
		status = DONE;
}

void RtiTask::rotatedCrop(QString output) {
	QDir destination(output);
	QStringList planes = destination.entryList(QStringList(QString("plane_*.jpg")), QDir::Files);
	for(QString plane: planes) {
		QString path = destination.absoluteFilePath(plane);
		QImage img;
		img.load(path, "JPG");
		img = crop.cropBoundingImage(img);
		bool saved = img.save(path, "jpg", parameters.quality);
	}
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
