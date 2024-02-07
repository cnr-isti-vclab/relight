#include "lightgeometry.h"
#include "relightapp.h"

#include <QVBoxLayout>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QGraphicsView>
#include <QGraphicsEllipseItem>

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
	grid->setColumnMinimumWidth(0, 200);
	grid->addWidget(new QLabel("Image width:"), 2, 0);
	grid->addWidget(image_width = new QDoubleSpinBox, 2, 1);
	grid->addWidget(new QLabel("cm"), 2, 2);
	connect(image_width, &QDoubleSpinBox::valueChanged, [&](double v) { qRelightApp->project().dome.imageWidth = v; });

	grid->addWidget(new QLabel("Diameter:"), 3, 0);
	grid->addWidget(diameter = new QDoubleSpinBox, 3, 1);
	grid->addWidget(new QLabel("cm"), 3, 2);
	connect(diameter, &QDoubleSpinBox::valueChanged, [&](double v) { qRelightApp->project().dome.domeDiameter = v; });

	grid->addWidget(new QLabel("Vertical offset:"), 4, 0);
	grid->addWidget(vertical_offset = new QDoubleSpinBox, 4, 1);
	grid->addWidget(new QLabel("cm"), 4, 2);
	connect(vertical_offset, &QDoubleSpinBox::valueChanged, [&](double v) { qRelightApp->project().dome.verticalOffset = v; });


	lights = new QGraphicsView(&scene);
	lights->setBackgroundBrush(Qt::black);
	lights->setMinimumSize(300, 300);
	lights->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	content->addWidget(lights);
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

	image_width->setValue(dome.imageWidth);
	diameter->setValue(dome.domeDiameter);
	vertical_offset->setValue(dome.verticalOffset);
}

void LightsGeometry::update(Dome dome) {
	qRelightApp->project().dome = dome;
	init();
}

void LightsGeometry::initLights() {
	qreal scale = 200;
	//scene goes from [-1, +1]x[-1, +1], view will just zoom on it
	qreal radius = scale/50;
	Dome &dome = qRelightApp->project().dome;
	for(Vector3f &dir: dome.directions) {
		QGraphicsEllipseItem *e = scene.addEllipse(dir[0]*scale, dir[1]*scale, radius, radius);
		e->setBrush(Qt::white);
	}

	qreal margin = scale/10;
	qreal side = scale + margin;
	lights->fitInView(QRectF(-side, -side, 2*side, 2*side), Qt::KeepAspectRatio);
}
