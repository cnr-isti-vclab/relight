#include "lightgeometry.h"
#include "relightapp.h"
#include "helpbutton.h"

#include <QVBoxLayout>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QRadioButton>
#include <QButtonGroup>
#include <QDoubleSpinBox>

LightsGeometry::~LightsGeometry() { if(group) delete group; }


LightsGeometry::LightsGeometry(QWidget *parent): QFrame(parent) {

	QGridLayout * content = new QGridLayout(this);


/*	content->addWidget( new QLabel("Number of images:"), 0, 0);
	content->addWidget(images_number = new QLabel, 0, 1); */

	content->addWidget(new QLabel("<h3>Lights geometrical configuration<h3>"), 0, 0);

	group = new QButtonGroup;

	content->addWidget(directional = new HelpRadio("Directional Lights", "lights/directional"), 1, 0);
	content->addWidget(sphere_approx = new HelpRadio("3D light positions on a sphere", "lights/3dsphere"), 2, 0);
	content->addWidget(three = new HelpRadio("3D light positions", "lights/3dposition"), 3, 0);
	group->addButton(directional->radioButton(), Dome::DIRECTIONAL);
	group->addButton(sphere_approx->radioButton(), Dome::SPHERICAL);
	group->addButton(three->radioButton(), Dome::LIGHTS3D);

	connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(setSpherical(QAbstractButton *)));


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

void LightsGeometry::setSpherical(QAbstractButton *button) {
	bool spherical = (button == sphere_approx->radioButton());
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

