#include "lightgeometry.h"
#include "relightapp.h"

#include <QVBoxLayout>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QRadioButton>
#include <QButtonGroup>
#include <QDoubleSpinBox>
#include <QDebug>

LightsGeometry::LightsGeometry(QWidget *parent): QFrame(parent) {

	QGridLayout * content = new QGridLayout(this);


/*	content->addWidget( new QLabel("Number of images:"), 0, 0);
	content->addWidget(images_number = new QLabel, 0, 1); */

	content->addWidget(new QLabel("<h3>Lights geometrical configuration<h3>"), 0, 0);

	group = new QButtonGroup;

	content->addWidget(directional = new QRadioButton("Directional Lights"), 1, 0);
	content->addWidget(sphere_approx = new QRadioButton("3D light positions on a sphere"), 2, 0);
	content->addWidget(three = new QRadioButton("3D light positions"), 3, 0);
	group->addButton(directional, Dome::DIRECTIONAL);
	group->addButton(sphere_approx, Dome::SPHERICAL);
	group->addButton(three, Dome::LIGHTS3D);

	connect(group, SIGNAL(buttonClicked(int)), this, SLOT(setSpherical(int)));


	QFrame *geometry = new QFrame;
	geometry->setFrameShape(QFrame::StyledPanel);

	content->addWidget(geometry, 1, 1, 3, 1);

	QGridLayout *grid = new QGridLayout(geometry);
	grid->setColumnMinimumWidth(0, 200);
	grid->addWidget(new QLabel("Image width:"), 2, 0);
	grid->addWidget(image_width = new QDoubleSpinBox, 2, 1);
	grid->addWidget(new QLabel("cm"), 2, 2);
	connect(image_width, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double v) { qRelightApp->project().dome.imageWidth = v; });

	grid->addWidget(new QLabel("Diameter:"), 3, 0);
	grid->addWidget(diameter = new QDoubleSpinBox, 3, 1);
	grid->addWidget(new QLabel("cm"), 3, 2);
	connect(diameter, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double v) { qRelightApp->project().dome.domeDiameter = v; });

	grid->addWidget(new QLabel("Vertical offset:"), 4, 0);
	grid->addWidget(vertical_offset = new QDoubleSpinBox, 4, 1);
	grid->addWidget(new QLabel("cm"), 4, 2);
	connect(vertical_offset, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double v) { qRelightApp->project().dome.verticalOffset = v; });

	//content->setColumnStretch(1, 1);
}

void LightsGeometry::setSpherical(int spherical) {

	diameter->setEnabled(spherical == Dome::SPHERICAL);
	vertical_offset->setEnabled(spherical == Dome::SPHERICAL);

	Dome &dome = qRelightApp->project().dome;
	dome.lightConfiguration = Dome::LightConfiguration(spherical);
	init();
}

void LightsGeometry::init() {
	Dome &dome = qRelightApp->project().dome;
	group->button(dome.lightConfiguration)->setChecked(true);

	bool spherical = dome.lightConfiguration == Dome::SPHERICAL;
	diameter->setEnabled(spherical);
	vertical_offset->setEnabled(spherical);

	image_width->setValue(dome.imageWidth);
	diameter->setValue(dome.domeDiameter);
	vertical_offset->setValue(dome.verticalOffset);
}

void LightsGeometry::update(Dome dome) {
	qRelightApp->project().dome = dome;
	init();
}

