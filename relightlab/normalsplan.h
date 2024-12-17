#ifndef NORMALSPLAN_H
#define NORMALSPLAN_H

#include "planrow.h"
#include "normalstask.h"

class QLabelButton;
class QLineEdit;
class QPushButton;
class QDoubleSpinBox;

class NormalsPlanRow: public PlanRow {
public:
	NormalsPlanRow(NormalsParameters &_parameters, QFrame *parent = nullptr);

	NormalsParameters &parameters;
};

#endif // NORMALSPLAN_H


class NormalsSourceRow: public NormalsPlanRow {
	Q_OBJECT
public:
	NormalsSourceRow(NormalsParameters &_parameters, QFrame *parent = nullptr);

	void setComputeSource(bool compute);
	void setSourcePath(QString path);
public slots:
	void selectOutput();

protected:
	QLabelButton *compute = nullptr;
	QLabelButton *file = nullptr;
	QLineEdit *input_path = nullptr;
	QPushButton *open = nullptr;
};

class NormalsFlattenRow: public NormalsPlanRow {
public:
	NormalsFlattenRow(NormalsParameters &_parameters, QFrame *parent = nullptr);
	void setFlattenMethod(FlatMethod method);
	void setFourierFrequency(double f);

	QLabelButton *none = nullptr;
	QLabelButton *radial = nullptr;
	QLabelButton *fourier = nullptr;

	QDoubleSpinBox *max_frequency = nullptr;
};

class NormalsSurfaceRow: public NormalsPlanRow {
public:
	NormalsSurfaceRow(NormalsParameters &_parameters, QFrame *parent = nullptr);
	void setSurfaceMethod(SurfaceIntegration surface);

	QLabelButton *none = nullptr;
	QLabelButton *fft = nullptr;
	QLabelButton *bni = nullptr;
	QLabelButton *assm = nullptr;

	QDoubleSpinBox *bni_k = nullptr;
	QDoubleSpinBox *assm_error = nullptr;
};


class NormalsExportRow: public NormalsPlanRow {
	Q_OBJECT
public:
	NormalsExportRow(NormalsParameters &parameters, QFrame *parent = nullptr);
	void setPath(QString path, bool emitting = false);
public slots:
	void selectOutput();
	void verifyPath();
	void suggestPath();

private:
	QLineEdit *path_edit;
};

