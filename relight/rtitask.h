#ifndef RTITASK_H
#define RTITASK_H

#define LIB_VIPS_ERR {const char* cError = vips_error_buffer(); \
    error = cError; \
    status = FAILED;}

#include "task.h"
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

class RtiTask: public Task {
	Q_OBJECT
public:
	enum Steps { RELIGHT, DEEPZOOM, TARZOOM, ITARZOOM };

	RtiTask();
	virtual ~RtiTask();
	virtual void run() override;
	virtual void pause() override;
	virtual void resume() override;
	virtual void stop() override;

public slots:
    bool progressed(std::string str, int percent) override;

	void relight();
	void toRTI();
	void fromRTI();
	void deepzoom();
	void tarzoom();
	void itarzoom();
	void openlime();

private:
	QMutex mutex;
	RtiBuilder *builder = nullptr;

};

#endif // RTITASK_H
