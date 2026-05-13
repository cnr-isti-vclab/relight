#include "calibrationdialog.h"
#include "caldomeframe.h"
#include "caldistortionframe.h"
#include "calflatfieldframe.h"
#include "callightsframe.h"
#include "../tabwidget.h"

#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QFileDialog>

CalibrationDialog::CalibrationDialog(QWidget *parent): QDialog(parent) {
	setWindowTitle("Dome Calibration");
	setMinimumSize(720, 540);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	tabs = new TabWidget;
	dome_frame = new CalDomeFrame;
	dome_frame->setSession(&session);
	tabs->addTab(dome_frame,                                 "Dome");
	tabs->addTab(distortion_frame  = new CalDistortionFrame, "Distortion");
	tabs->addTab(lights_frame      = new CalLightsFrame,     "Lights");
	lights_frame->setSession(&session);
	tabs->addTab(flatfield_frame   = new CalFlatfieldFrame,  "Flatfield");

	layout->addWidget(tabs, 1);

	QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
	buttons->setContentsMargins(8, 4, 8, 8);

	QPushButton *save_btn = buttons->addButton("Save dome…", QDialogButtonBox::ActionRole);
	connect(save_btn, &QPushButton::clicked, this, &CalibrationDialog::saveDome);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	layout->addWidget(buttons);
}

void CalibrationDialog::saveDome() {
	QString path = QFileDialog::getSaveFileName(
		this, "Save dome configuration", QString(),
		"Dome files (*.dome);;JSON files (*.json);;All files (*)");
	if (!path.isEmpty())
		session.dome.save(path);
}
