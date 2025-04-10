#ifndef RTIFRAME_H
#define RTIFRAME_H

#include <QFrame>
//TODO we should separate RTI definitions from actual implementation (materials etc).
#include "../src/rti.h"
#include "rtitask.h"

class RtiRecents;
class RtiParameters;

class RtiBasisRow;
class RtiColorSpaceRow;
class RtiPlanesRow;
class RtiFormatRow;
class RtiQualityRow;
class RtiWebLayoutRow;
class RtiExportRow;
class ZoomOverview;

class RtiFrame: public QFrame {
	Q_OBJECT
public:
	RtiFrame(QWidget *parent = nullptr);


	RtiParameters parameters;

public slots:
	void init();
	void exportRti();

	void basisChanged();
	void colorspaceChanged();
	void nplanesChanged();
	void formatChanged();
	void qualityChanged();
	void layoutChanged();

	void updateCrop(QRect rect, float angle);

signals:
	void processStarted();


private:
	RtiRecents *recents;

	RtiBasisRow *basis_row = nullptr;
	RtiColorSpaceRow *colorspace_row = nullptr;
	RtiPlanesRow *planes_row = nullptr;
	RtiFormatRow *format_row = nullptr;
	RtiQualityRow *quality_row = nullptr;
	RtiWebLayoutRow *layout_row = nullptr;
	RtiExportRow *export_row = nullptr;

	ZoomOverview *zoom_view =  nullptr;

	void updateNPlanes();
};


#endif // RTIFRAME_H
