#include "lightgeometry.h"
#include "relightapp.h"
#include "helpbutton.h"
#include "directionsview.h"

#include <QVBoxLayout>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QTextEdit>
#include <QRadioButton>
#include <QPushButton>
#include <QButtonGroup>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QFileDialog>

LightsGeometry::~LightsGeometry() { if(group) delete group; }


LightsGeometry::LightsGeometry(QWidget *parent): QFrame(parent) {

	QVBoxLayout *page = new QVBoxLayout(this);

	page->addWidget(new QLabel("<h3>Current dome configuration<h3>"));
	//page->addSpacing(10);

	QGridLayout * content = new QGridLayout();
	page->addLayout(content);

	content->addWidget( new QLabel("Number of images:"), 0, 0);
	content->addWidget(images_number = new QSpinBox, 0, 1);

	content->addWidget(new QLabel("Notes:"), 1, 0);
	content->addWidget(notes = new QTextEdit, 1, 1);
	notes->setMaximumHeight(100);


	group = new QButtonGroup;

	content->addWidget(directional = new HelpRadio("Directional Lights", "lights/directional"), 2, 0);
	content->addWidget(sphere_approx = new HelpRadio("3D light positions on a sphere", "lights/3dsphere"), 3, 0);
	content->addWidget(three = new HelpRadio("3D light positions", "lights/3dposition"), 4, 0);
	group->addButton(directional->radioButton(), Dome::DIRECTIONAL);
	group->addButton(sphere_approx->radioButton(), Dome::SPHERICAL);
	group->addButton(three->radioButton(), Dome::LIGHTS3D);

	connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(setSpherical(QAbstractButton *)));


	QFrame *geometry = new QFrame;
	geometry->setFrameShape(QFrame::StyledPanel);

	content->addWidget(geometry, 2, 1, 3, 1);

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


	/* it seems basically impossible to have a widget scale while preserving aspect ratio, bummer */

	content->setSpacing(20);
	directions_view = new DirectionsView;
	content->addWidget(directions_view, 0, 2, 5, 1, Qt::AlignBottom);
	directions_view->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	directions_view->setMaximumSize(300, 300);
	directions_view->setMinimumSize(300, 300);

	page->addSpacing(20);

	QPushButton *save_dome = new QPushButton(QIcon::fromTheme("save"), "Export as dome...");
	page->addWidget(save_dome, Qt::AlignRight);
	save_dome->setProperty("class", "large");

	connect(save_dome, SIGNAL(clicked()), this, SLOT(exportDome()));

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
