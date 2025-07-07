#ifndef BRDFPLAN_H
#define BRDFPLAN_H

#include "planrow.h"
#include "brdftask.h"

class QSlider;
class QLabel;
class QLineEdit;

class BrdfPlanRow: public PlanRow {
public:
	BrdfPlanRow(BrdfParameters &_parameters, QFrame *parent = nullptr);
	BrdfParameters &parameters;
};

class BrdfMedianRow: public BrdfPlanRow {
public:
	BrdfMedianRow(BrdfParameters &parameters, QFrame *parent = nullptr);
	BrdfParameters &parameters;
	QSlider *median_slider = nullptr;
	QLabel *median_text = nullptr;
};

class BrdfExportRow: public BrdfPlanRow {
	Q_OBJECT
public:
	BrdfExportRow(BrdfPlanRow &parameters, QFrame *parent = nullptr);
	void setPath(QString path, bool emitting = false);
public slots:
	void selectOutput();
	void verifyPath();
	void suggestPath();

private:
	QLineEdit *path_edit;
};


#endif // BRDFPLAN_H
