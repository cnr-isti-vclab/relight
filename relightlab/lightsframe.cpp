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

LightsFrame::LightsFrame() {
	QHBoxLayout *page = new QHBoxLayout(this);
	QVBoxLayout *content = new QVBoxLayout;
	page->addStretch(1);
	page->addLayout(content, 5);
	page->addStretch(1);

	content->addWidget(new QLabel("<h2>Lights direction setup</h2>"));
	content->addSpacing(30);

	QTabWidget *choice = new QTabWidget;
	content->addWidget(choice);


	choice->addTab(sphere_panel = new SpherePanel, "Sphere");
	choice->addTab(dome_panel = new DomePanel, "Dome");

	content->addSpacing(30);

	geometry = new LightsGeometry;
	content->addWidget(geometry);

	QPushButton *export_dome = new QPushButton(QIcon::fromTheme("save"), "Export as dome...");
	content->addWidget(export_dome, Qt::AlignRight);

	content->addStretch();

	connect(sphere_panel, SIGNAL(accept(Dome)), this, SLOT(init(Dome)));
	connect(dome_panel, SIGNAL(accept(Dome)), this, SLOT(init(Dome)));

	connect(export_dome, SIGNAL(clicked()), this, SLOT(exportDome()));
}

void LightsFrame::init() {
	sphere_panel->init();
	dome_panel->init();
	geometry->init();
}

void LightsFrame::init(Dome _dome) {
	geometry->update(_dome);
	dome = _dome;
}

void LightsFrame::exportDome() {
	QString filename = QFileDialog::getSaveFileName(this, "Select a dome file", qRelightApp->lastProjectDir(), "*.dome");
	if(filename.isNull())
		return;
	if(!filename.endsWith(".dome"))
		filename += ".dome";
	//TODO Basic checks, label is a problem (use filename!
	dome.save(filename);
	qRelightApp->addDome(filename);
}


