#ifndef BRDFFRAME_H
#define BRDFFRAME_H

#include <QFrame>
#include "planrow.h"
#include "brdftask.h"

class ZoomOverview;
class QSlider;
class QLabel;

class BrdfMedianRow: public PlanRow {
public:
	BrdfMedianRow(BrdfParameters &parameters, QFrame *parent = nullptr);
	BrdfParameters &parameters;
	QSlider *median_slider = nullptr;
	QLabel *median_text = nullptr;
};

class BrdfFrame: public QFrame {
	Q_OBJECT
public:
	BrdfFrame(QWidget *parent = nullptr);
	void clear();
	void init();

public slots:

signals:
	void processStarted();

private:
	BrdfMedianRow *median_row = nullptr;
	ZoomOverview *zoom_view =  nullptr;
	BrdfParameters parameters;

};

#endif // BRDFFRAME_H
