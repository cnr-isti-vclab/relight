#ifndef NORMALSPLAN_H
#define NORMALSPLAN_H

#include "planrow.h"
#include "../src/normals/normalstask.h"

class QLabelButton;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QDoubleSpinBox;
class QGridLayout;

class NormalsPlanRow: public PlanRow {
	Q_OBJECT
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
	void updateSize();
public slots:
	void selectOutput();

signals:
	void sourceSizeChanged(int w, int h);

protected:
	QLabelButton *compute = nullptr;
	QLabelButton *file = nullptr;
	QLineEdit *input_path = nullptr;
	QPushButton *open = nullptr;
	QFrame *input_frame = nullptr;
};

class NormalsFlattenRow: public NormalsPlanRow {
public:
	NormalsFlattenRow(NormalsParameters &_parameters, QFrame *parent = nullptr);
	void setFlattenMethod(FlatMethod method);
	void setFourierFrequency(double f);
	void setBlurFrequency(double f);

	QLabelButton *none = nullptr;
	QLabelButton *radial = nullptr;
	QLabelButton *fourier = nullptr;
	QLabelButton *gaussian = nullptr;

	QDoubleSpinBox *max_frequency = nullptr;
	QDoubleSpinBox *blur_percentage = nullptr;

	QFrame *frequency_frame = nullptr;
	QFrame *blur_frame = nullptr;
};

class NormalsSurfaceRow: public NormalsPlanRow {
	Q_OBJECT
public:
	NormalsSurfaceRow(NormalsParameters &_parameters, QFrame *parent = nullptr);
	void setSurfaceMethod(SurfaceIntegration surface);

public slots:
	void updateDimensions(int w, int h);
	void setDownsample(float down, int w, int h);

private:
	QLabelButton *none = nullptr;
	QLabelButton *fft = nullptr;
	QLabelButton *bni = nullptr;
	QLabelButton *assm = nullptr;

	QDoubleSpinBox *bni_k = nullptr;
	QDoubleSpinBox *assm_error = nullptr;

	QDoubleSpinBox *downsample = nullptr;
	QSpinBox *width = nullptr;
	QSpinBox *height = nullptr;
	QFrame *downsample_frame = nullptr;
	QFrame *bni_frame = nullptr;
	QFrame *assm_frame = nullptr;

	int base_width = 1;
	int base_height = 1;
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

