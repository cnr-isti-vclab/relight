#include "lightsframe.h"
#include "relightapp.h"
#include "domepanel.h"
#include "spherepanel.h"
#include "lightgeometry.h"

#include <QVBoxLayout>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QTabBar>
#include <QFileDialog>

LightsFrame::LightsFrame() {
	QHBoxLayout *page = new QHBoxLayout(this);
	QVBoxLayout *content = new QVBoxLayout;
	page->addStretch(1);
	page->addLayout(content, 5);
	page->addStretch(1);

	content->addWidget(new QLabel("<h2>Lights direction setup</h2>"));
	content->addSpacing(30);

	choice = new QTabWidget;
	content->addWidget(choice);


	choice->addTab(sphere_panel = new SpherePanel, "Sphere");
	choice->addTab(dome_panel = new DomePanel, "Dome");

	content->addSpacing(30);

	geometry = new LightsGeometry;
	content->addWidget(geometry);

	content->addStretch();

	connect(sphere_panel, SIGNAL(accept(Dome)), this, SLOT(setDome(Dome)));
	connect(dome_panel, SIGNAL(updated()), this, SLOT(setSpheres()));
}

void LightsFrame::clear() {
	sphere_panel->clear();
}

void LightsFrame::init() {
	bool useSphere = qRelightApp->project().spheres.size();
	choice->setCurrentIndex(useSphere? 0 : 1);

	sphere_panel->init();
	dome_panel->init();
	geometry->init();
}

void LightsFrame::setDome(Dome _dome) {
	dome = _dome;
	geometry->updateDome(_dome);
}

void LightFrame::setSpheres() {
	geometry->updateSpheres();
}


