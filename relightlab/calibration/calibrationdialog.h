#ifndef CALIBRATIONDIALOG_H
#define CALIBRATIONDIALOG_H

#include <QDialog>
#include "../../src/calibration/calibrationsession.h"

class TabWidget;
class CalImagesFrame;
class CalHistogramFrame;
class CalDistortionFrame;
class CalFlatfieldFrame;
class CalLightsFrame;

class CalibrationDialog: public QDialog {
	Q_OBJECT
public:
	explicit CalibrationDialog(QWidget *parent = nullptr);

private:
	CalibrationSession  session;
	TabWidget          *tabs             = nullptr;
	CalImagesFrame     *images_frame     = nullptr;
	CalHistogramFrame  *histogram_frame  = nullptr;
	CalDistortionFrame *distortion_frame = nullptr;
	CalFlatfieldFrame  *flatfield_frame  = nullptr;
	CalLightsFrame     *lights_frame     = nullptr;
};

#endif // CALIBRATIONDIALOG_H
