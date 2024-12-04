#ifndef NORMALSFRAME_H
#define NORMALSFRAME_H

#include <QFrame>
#include "normalstask.h"

class QCheckBox;
class QRadioButton;
class QSpinBox;
class QDoubleSpinBox;

class NormalsSourceRow;
class NormalsFlattenRow;
class NormalsSurfaceRow;
class NormalsExportRow;
class NormalsFrame: public QFrame {
	Q_OBJECT
public:
	NormalsFrame(QWidget *parent = nullptr);
	NormalsParameters parameters;

public slots:
	void save();

signals:
	void processStarted();

private:
	NormalsSourceRow *source_row = nullptr;
	NormalsFlattenRow *flatten_row = nullptr;
	NormalsSurfaceRow *surface_row = nullptr;
	NormalsExportRow *export_row = nullptr;

	QRadioButton *jpg = nullptr;
	QRadioButton *png = nullptr;
	QCheckBox *tif = nullptr;
	QCheckBox *ply = nullptr;
	QDoubleSpinBox *discontinuity = nullptr;
	QCheckBox *radial = nullptr;
	QCheckBox *fourier = nullptr;
	QSpinBox *fourier_radius = nullptr;
};

#endif // NORMALSFRAME_H
