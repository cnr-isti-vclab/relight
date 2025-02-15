#include "lightsframe.h"
#include "relightapp.h"
#include "domepanel.h"
#include "lightgeometry.h"

#include <QVBoxLayout>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QTabBar>
#include <QFileDialog>
#include <QDoubleSpinBox>
#include <QScrollArea>

LightsFrame::LightsFrame() {
	QHBoxLayout *page = new QHBoxLayout(this);
	QVBoxLayout *content = new QVBoxLayout;
	page->addStretch(1);
	page->addLayout(content, 5);
	page->addStretch(1);

	content->addWidget(new QLabel("<h2>Lights direction setup</h2>"));
	content->addSpacing(30);

	//content->addWidget(sphere_panel = new SpherePanel(this));
	//content->addSpacing(30);

	content->addWidget(dome_panel = new DomePanel(this));
	content->addSpacing(30);

	geometry = new LightsGeometry(this);
	content->addWidget(geometry);

	content->addStretch();

//	connect(sphere_panel, SIGNAL(updated()), geometry, SLOT(setFromSpheres()));
	connect(dome_panel, SIGNAL(updated()), geometry, SLOT(init()));
	connect(dome_panel, SIGNAL(useSpheres()), this, SLOT(updateSphere()));
}

void LightsFrame::clear() {
//	sphere_panel->clear();
}

void LightsFrame::updateSphere() {
	geometry->setFromSpheres();
}

void LightsFrame::init() {
//	bool useSphere = qRelightApp->project().spheres.size();

//	sphere_panel->init();
	dome_panel->init();
	geometry->init();
}
void LightsFrame::setPixelSize() {
	geometry->image_width->setValue(qRelightApp->project().dome.imageWidth);
}



