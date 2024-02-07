#include "lightgeometry.h"
#include "relightapp.h"

#include <QVBoxLayout>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QDoubleSpinBox>

LightsGeometry::LightsGeometry(QWidget *parent): QFrame(parent) {
	QVBoxLayout *content = new QVBoxLayout(this);
	content->addWidget(images_number = new QLabel("Number of images:"));

	content->addSpacing(20);
	content->addWidget(sphere_approx = new QCheckBox("Enable 3D light positions on a sphere"), 0);
	connect(sphere_approx, SIGNAL(stateChanged(int)), this, SLOT(setSpherical(int)));

	QFrame *geometry = new QFrame;
	geometry->setFrameShape(QFrame::StyledPanel);

	content->addWidget(geometry);

	QGridLayout *grid = new QGridLayout(geometry);
	grid->addWidget(new QLabel("Image width:"), 2, 0);
	grid->addWidget(image_width = new QDoubleSpinBox, 2, 1);
	grid->addWidget(new QLabel("cm"), 2, 2);

	grid->addWidget(new QLabel("Diameter:"), 3, 0);
	grid->addWidget(diameter = new QDoubleSpinBox, 3, 1);
	grid->addWidget(new QLabel("cm"), 3, 2);

	grid->addWidget(new QLabel("Vertical offset:"), 4, 0);
	grid->addWidget(vertical_offset = new QDoubleSpinBox, 4, 1);
	grid->addWidget(new QLabel("cm"), 4, 2);

	content->addStretch();
}

void LightsGeometry::setSpherical(int spherical) {
	Dome &dome = qRelightApp->project().dome;
	if(spherical)
		dome.lightConfiguration = Dome::SPHERICAL;
	else
		dome.lightConfiguration = Dome::DIRECTIONAL;

	init();
}

void LightsGeometry::init() {
	Dome &dome = qRelightApp->project().dome;
	bool spherical = dome.lightConfiguration == Dome::SPHERICAL;
	sphere_approx->setChecked(spherical);
	diameter->setEnabled(spherical);
	vertical_offset->setEnabled(spherical);
}

