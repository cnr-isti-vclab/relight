#include <vips/vips.h>

#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QSettings>
#include <QRect>

#include "rtitask.h"
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
void deepzoom();
void tarzoom();
void itarzoom();


void RtiTask::run() {
	status = RUNNING;
	QStringList steps = (*this)["steps"].value.toStringList();
	for(auto step: steps) {
		if(step == "relight")
			relight();
		else if(step == "toRTI")
			toRTI();
		else if(step == "fromRTI")
			fromRTI();
		else if(step == "deepzoom")
			deepzoom();
		else if(step == "tarzoom")
			tarzoom();
		else if(step == "itarzoom")
			itarzoom();
		else if(step == "openlime")
			openlime();
	}
	if(status != FAILED)
		status = DONE;
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
void RtiTask::deepzoom() {
	int nplanes = nPlanes(output);
	int quality= (*this)["quality"].value.toInt();

    // Deep zoom every plane
    for(int plane = 0; plane < nplanes; plane++)
    {
        // Load image, setup output folder for this plane
        QString fileName = (QStringList() << QString("%1/plane_%2").arg(output).arg(plane) << QString(".jpg")).join("");
        VipsImage* image = vips_image_new_from_file(fileName.toStdString().c_str(), NULL);
        if (image == NULL)
            LIB_VIPS_ERR
        QString folderName = QString("%1\\plane_%2").arg(output).arg(plane).toStdString().c_str();

        //qDebug() << "Image path: " << fileName;
        //qDebug() << "Save path: " << folderName;

        // Call dzsave and create the deepzoom tiles
        if (image == NULL || vips_dzsave(image, folderName.toStdString().c_str(),
            "overlap", 0,
            "tile_size", 256,
            "layout", VIPS_FOREIGN_DZ_LAYOUT_DZ,
            "depth", VIPS_FOREIGN_DZ_DEPTH_ONETILE,
            "suffix", QString(".jpg[Q=%1]").arg(quality).toStdString().c_str(),
            NULL) != 0)
        {
            LIB_VIPS_ERR
        }

        // Update progress bar
		if(!progressed("Deepzoom:", 100*(plane+1)/nplanes))
			break;
    }

    /**
    if (vips_dzsave(ImOutTotal, "C:\\VIPS_Imaging\\Colon\\My_Tiles",

        //"compression" , 1,
        NULL))   //Here the run-time throw the exception!!!
    {
        const char* cError = vips_error_buffer();

        vips_error_exit(cError);
    }
     */
}


void RtiTask::tarzoom() {
	int nplanes = nPlanes(output);
    // For each plane file
    for(int plane = 0; plane < nplanes; plane++)
    {
		/** TODO: handle errors in deep zoom, stick to old names */
        /** Run a single tarzoom script:
         *
         *  - Keep an index object containing the data about the tarzoom
         *  - Read the dzi contents
         *  - Create a .tzb file
         *  - Find all the folders that end with "_files" (deep zoom folders)
         *  - For each of those folders:
         *      - Get all the folders (0 to 5)
         *      - For each of those folders:
         *          - files = [(f.name, f.path) for f in os.scandir(level[1]) if f.is_file()]
         *          - Has this been tested? Shouldn't it be ... scandir(level) ... ?
         *          - For each of those files:
         *              - Computations in the file
         */
		runPythonScript("tarzoom.py", QStringList() << QString("plane_%1").arg(plane), output);
		if(status == FAILED)
			return;

		if(!progressed("Tarzoom:", 100*(plane+1)/nplanes))
			break;
	}
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
