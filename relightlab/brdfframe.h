#ifndef BRDFFRAME_H
#define BRDFFRAME_H

#include <QFrame>
#include "planrow.h"
#include "brdftask.h"

class ZoomOverview;
class BrdfMedianRow;
class BrdfExportRow;


class BrdfFrame: public QFrame {
	Q_OBJECT
public:
	BrdfFrame(QWidget *parent = nullptr);

public slots:
	void save();
	void init();
	void updateCrop(Crop crop);


signals:
	void processStarted();

private:
	BrdfMedianRow *median_row = nullptr;
	BrdfExportRow *export_row = nullptr;
	ZoomOverview *zoom_view =  nullptr;
	BrdfParameters parameters;

};

#endif // BRDFFRAME_H
