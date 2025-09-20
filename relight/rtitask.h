#ifndef RTITASK_H
#define RTITASK_H

#include "../src/task.h"
#include "../src/rti.h"
#include "../src/project.h"
#include <QMutex>

class RtiBuilder;

/* steps could be:
 *   relight: creates an RTI in relight format
 *   toRTI: converts a relight to an .rti format
 *   fromRTI: converts an .rti to relight
 *   deepzoom: splits relight in tiles
 *   tarzoom: merges deepzoom tiles
 *   itarzoom: merges tarzoom in a single itarzoom
 *   openlime: add openlime js css, html for viewer
 */

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
};

class RtiTask: public Task {
	Q_OBJECT
public:
	Project project;
	RtiParameters parameters;

	RtiTask(const Project &_project);
    virtual ~RtiTask();
    virtual void run() override;

public slots:
	void relight(bool commonMinMax = false, bool saveLegacy = false); //use true for .rti and .ptm
	void toRTI();
    void fromRTI();
	void openlime();

private:
	RtiBuilder *builder = nullptr;

};

#endif // RTITASK_H
