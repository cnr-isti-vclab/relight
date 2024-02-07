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
#include <QDebug>
#include <QFileDialog>

Card::Card(QString title, QString subtitle, QWidget *parent): QFrame(parent) {
	setFrameShape(QFrame::StyledPanel);
	QVBoxLayout *layout = new QVBoxLayout(this);

	QPushButton *button = new QPushButton(title);

	QObject::connect(button, SIGNAL(clicked()), this, SLOT(click()));
	button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	setStyleSheet("QPushButton { font-size: 24px; padding:20px}");

	layout->addWidget(button);
	layout->addSpacing(30);
	QLabel *body = new QLabel(subtitle);
	body->setAlignment(Qt::AlignCenter);
	layout->addWidget(body);
}

LightsFrame::LightsFrame() {
	QVBoxLayout *content = new QVBoxLayout(this);

	QTabWidget *choice = new QTabWidget;
	content->addWidget(choice);

	lp = new LpPanel;
	choice->addTab(lp, "Load .lp file");

	sphere = new SpherePanel;
	choice->addTab(sphere, "Identify reflective spheres");

	dome = new DomePanel;
	choice->addTab(dome, "Select an existing dome configuration");

	geometry = new LightsGeometry;
	content->addWidget(geometry);

	connect(lp, &LpPanel::accept, geometry, &LightsGeometry::update);
	connect(sphere, &SpherePanel::accept, geometry, &LightsGeometry::update);
	connect(dome, &DomePanel::accept, geometry, &LightsGeometry::update);
}

void LightsFrame::init() {
	sphere->init();
	dome->init();
	geometry->init();
}



SpherePanel::SpherePanel(QWidget *parent): QFrame(parent) {
	QVBoxLayout *content = new QVBoxLayout(this);
	this->setLayout(content);

	QLabel *title = new QLabel("<h2>Sphere light directions</h2>");
	content->addWidget(title);
}

void SpherePanel::init() {
}


