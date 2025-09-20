#ifndef RTITASK_H
#define RTITASK_H

#include "task.h"
#include "../src/project.h"
#include "../src/rti.h"
#include <QMutex>

class RtiBuilder;

class RtiParameters {
public:

	enum Format { RTI = 0, WEB = 1, IIP = 2 };
	enum WebLayout { PLAIN = 0, DEEPZOOM = 1, TARZOOM = 2, ITARZOOM = 3 };

	Rti::Type basis = Rti::PTM;
	Rti::ColorSpace colorspace = Rti::RGB;
	int nplanes = 18;
	int nchroma = 0;

	Format format = WEB;
	WebLayout web_layout = PLAIN;

	bool lossless = false; //used only for RTI format;

	bool iiif_manifest = false;  //TODO
	bool openlime = true; //include openlime viewer //TODO: might want different interfaces.

	int quality = 95;
	QString path;

	QString summary();
};

class RtiTask: public Task {
	Q_OBJECT
public:
	RtiParameters parameters;
	Crop crop;
	//QSize img_size;

	RtiTask();
	virtual ~RtiTask();

	void setProject(Project &_project);
	void setParameters(RtiParameters &p);
	void rotatedCrop(QString output);
	virtual void run() override;


public slots:

	//void relight(bool commonMinMax = false, bool saveLegacy = false); //use true for .rti and .ptm
	//void toRTI();
	//void fromRTI();
	void openlime();

private:
	RtiBuilder *builder = nullptr;

};

#endif // RTITASK_H
