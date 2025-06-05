#ifndef BRDFFRAME_H
#define BRDFFRAME_H

#include <QFrame>
#include "planrow.h"

class ZoomOverview;


class BrdfMedianRow: public PlanRow {
public:
	BrdfMedianRow(QWidget *parent = nullptr) {}
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

};

#endif // BRDFFRAME_H
