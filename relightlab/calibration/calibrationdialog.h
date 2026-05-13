#ifndef CALIBRATIONDIALOG_H
#define CALIBRATIONDIALOG_H

#include <QDialog>
#include "../../src/calibration/calibrationsession.h"

class TabWidget;
class CalDomeFrame;
class CalDistortionFrame;
class CalFlatfieldFrame;
class CalLightsFrame;

class CalibrationDialog: public QDialog {
	Q_OBJECT
public:
	explicit CalibrationDialog(QWidget *parent = nullptr);

public slots:
	void saveDome();

private:
	CalibrationSession  session;
	TabWidget          *tabs             = nullptr;
	CalDomeFrame       *dome_frame       = nullptr;
	CalDistortionFrame *distortion_frame = nullptr;
	CalFlatfieldFrame  *flatfield_frame  = nullptr;
	CalLightsFrame     *lights_frame     = nullptr;
};

#endif // CALIBRATIONDIALOG_H
