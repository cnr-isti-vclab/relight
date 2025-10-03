#include "lightgeometry.h"
#include "relightapp.h"
#include "helpbutton.h"
#include "directionsview.h"
#include "../src/sphere.h"

#include <QVBoxLayout>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QRadioButton>
#include <QPushButton>
#include <QButtonGroup>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QFileDialog>

using namespace std;

LightsGeometry::~LightsGeometry() { if(group) delete group; }


LightsGeometry::LightsGeometry(QWidget *parent): QFrame(parent) {

	QVBoxLayout *page = new QVBoxLayout(this);

	page->addWidget(new QLabel("<h3>Current lights configuration<h3>"));
	//page->addSpacing(10);

	QGridLayout * content = new QGridLayout();
	page->addLayout(content);

	content->addWidget( new QLabel("Filename:"), 0, 0);
	content->addWidget(filename = new QLineEdit, 0, 1);
	filename->setEnabled(false);

	content->addWidget( new QLabel("Number of images:"), 1, 0);
	content->addWidget(images_number = new QSpinBox, 1, 1);
	images_number->setRange(1, 1024);
	images_number->setEnabled(false);

	content->addWidget(new QLabel("Notes:"), 2, 0);
	content->addWidget(notes = new QTextEdit, 2, 1);
	notes->setMaximumHeight(100);
	connect(notes, &QTextEdit::textChanged, [&]() { qRelightApp->project().dome.notes = notes->toPlainText(); });



	group = new QButtonGroup;

	content->addWidget(directional = new HelpRadio("Directional Lights", "lights/directional"), 3, 0);
	content->addWidget(sphere_approx = new HelpRadio("3D light positions on a sphere", "lights/3dsphere"), 4, 0);
	content->addWidget(lights3d = new HelpRadio("3D light positions", "lights/3dposition"), 5, 0);
	group->addButton(directional->radioButton(), Dome::DIRECTIONAL);
	group->addButton(sphere_approx->radioButton(), Dome::SPHERICAL);
	group->addButton(lights3d->radioButton(), Dome::LIGHTS3D);

	connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(setSpherical(QAbstractButton *)));

	QFrame *geometry = new QFrame;
	geometry->setFrameShape(QFrame::StyledPanel);

	content->addWidget(geometry, 3, 1, 3, 1);

	QGridLayout *grid = new QGridLayout(geometry);
	grid->setColumnMinimumWidth(0, 200);
	grid->addWidget(new QLabel("Image width:"), 2, 0);
	grid->addWidget(image_width = new QDoubleSpinBox, 2, 1);
	image_width->setRange(0, 10000);
	grid->addWidget(new QLabel("mm"), 2, 2);
	connect(image_width, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double v) {
		auto &project = qRelightApp->project();
		project.dome.imageWidth = v;
		project.pixelSize = project.dome.imageWidth/project.imgsize.width();
	});

	grid->addWidget(new QLabel("Dome radius:"), 3, 0);
	grid->addWidget(radius = new QDoubleSpinBox, 3, 1);
	radius->setRange(0, 10000);
	grid->addWidget(new QLabel("mm"), 3, 2);
	connect(radius, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double v) { qRelightApp->project().dome.domeDiameter = v*2.0; });

	grid->addWidget(new QLabel("Vertical offset:"), 4, 0);
	grid->addWidget(vertical_offset = new QDoubleSpinBox, 4, 1);
	vertical_offset->setRange(-1000, 1000);
	grid->addWidget(new QLabel("mm"), 4, 2);
	connect(vertical_offset, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double v) { qRelightApp->project().dome.verticalOffset = v; });


	/* it seems basically impossible to have a widget scale while preserving aspect ratio, bummer */

	content->setSpacing(20);
	directions_view = new DirectionsView;
	content->addWidget(directions_view, 0, 2, 3, 1, Qt::AlignBottom);
	directions_view->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	directions_view->setMaximumSize(200, 200);
	directions_view->setMinimumSize(200, 200);

	page->addSpacing(30);
}

void LightsGeometry::setSpherical(QAbstractButton *button) {
	Dome &dome = qRelightApp->project().dome;
	dome.lightConfiguration = Dome::DIRECTIONAL;

	bool spherical = (button == sphere_approx->radioButton());
	radius->setEnabled(spherical == Dome::SPHERICAL);
	vertical_offset->setEnabled(spherical == Dome::SPHERICAL);

	if(button == sphere_approx->radioButton()) {
		dome.lightConfiguration = Dome::SPHERICAL;
	} else if(button == lights3d->radioButton()) {
		dome.lightConfiguration = Dome::LIGHTS3D;
	}

	init();
}

void LightsGeometry::init() {
	Dome &dome = qRelightApp->project().dome;

	filename->setText(dome.label);
	notes->setText(dome.notes);
	images_number->setValue(dome.lightsCount());
	group->button(dome.lightConfiguration)->setChecked(true);

	bool spherical = dome.lightConfiguration == Dome::SPHERICAL;
	radius->setEnabled(spherical);
	vertical_offset->setEnabled(spherical);

	image_width->setValue(dome.imageWidth);
	radius->setValue(dome.domeDiameter/2.0);
	vertical_offset->setValue(dome.verticalOffset);
	directions_view->initFromDome(dome);
}


void LightsGeometry::setFromSpheres() {
	//get spheres & lens from project
	Project &project = qRelightApp->project();
	//call appropriate compute directions/positions
	Dome &dome = project.dome;
	dome.label = "";
	dome.fromSpheres(project.images, project.spheres, project.lens);

	init();
}

void LightsGeometry::exportDome() {
	QString filename = QFileDialog::getSaveFileName(this, "Select a dome file", qRelightApp->lastProjectDir(), "*.dome");
	if(filename.isNull())
		return;
	if(!filename.endsWith(".dome"))
		filename += ".dome";
	//TODO Basic checks, label is a problem (use filename!
	Dome &dome = qRelightApp->project().dome;
	dome.save(filename);
	qRelightApp->addDome(filename);
}
