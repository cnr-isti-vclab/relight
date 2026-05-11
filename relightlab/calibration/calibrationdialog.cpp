#include "calibrationdialog.h"
#include "calimagesframe.h"
#include "calhistogramframe.h"
#include "caldistortionframe.h"
#include "calflatfieldframe.h"
#include "callightsframe.h"
#include "../tabwidget.h"

#include <QVBoxLayout>
#include <QDialogButtonBox>

CalibrationDialog::CalibrationDialog(QWidget *parent): QDialog(parent) {
	setWindowTitle("Dome Calibration");
	setMinimumSize(720, 540);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	tabs = new TabWidget;
	tabs->addTab(images_frame      = new CalImagesFrame(&session), "Images");
	tabs->addTab(histogram_frame   = new CalHistogramFrame, "Histogram");
	tabs->addTab(distortion_frame  = new CalDistortionFrame,"Distortion");
	tabs->addTab(flatfield_frame   = new CalFlatfieldFrame, "Flatfield");
	tabs->addTab(lights_frame      = new CalLightsFrame,    "Lights");

	layout->addWidget(tabs, 1);

	QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
	buttons->setContentsMargins(8, 4, 8, 8);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	layout->addWidget(buttons);
}
