#include "lightsframe.h"

#include <QVBoxLayout>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QLabel>
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
	addWidget(createChoiceFrame());
	lp = new LpFrame();
	addWidget(lp);
	sphere = new SphereFrame();
	addWidget(sphere);
	dome = new DomeFrame();
	addWidget(dome);
}

void LightsFrame::init() {
	lp->init();
	sphere->init();
	dome->init();
}

QFrame *LightsFrame::createChoiceFrame() {
	QFrame *lights_choice = new QFrame();
	//lights_choice->setObjectName("lights_choice");

	QVBoxLayout *content = new QVBoxLayout(lights_choice);

	content->addStretch(1);
	QHBoxLayout *buttons = new QHBoxLayout();
	buttons->setSpacing(30);

	content->addLayout(buttons, 0);

	buttons->addStretch(1);

	Card *lp = new Card("LP", "<p>Load a file (.lp) containging the light directions.</p>");
	connect(lp, SIGNAL(clicked()), this, SLOT(showLp()));
	buttons->addWidget(lp, 1);

	Card *sphere = new Card("Sphere", "<p>Identify one or more reflective spheres</p>");
	connect(sphere, SIGNAL(clicked()), this, SLOT(showSphere()));
	buttons->addWidget(sphere, 1);

	Card *dome = new Card("Dome", "<p>Select a preconfigure dome</p>");
	connect(dome, SIGNAL(clicked()), this, SLOT(showDome()));
	buttons->addWidget(dome, 1);

	buttons->addStretch(1);

	content->addStretch(2);
	return lights_choice;
}

void LightsFrame::showChoice() {

	setCurrentIndex(0);
}
void LightsFrame::showLp() {
	setCurrentWidget(lp);
}

void LightsFrame::showSphere() {
	setCurrentIndex(2);
}

void LightsFrame::showDome() {
	setCurrentIndex(3);
}



LpFrame::LpFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *content = new QVBoxLayout(this);
	content->setContentsMargins(31, 31, 31, 31);
	this->setLayout(content);

	QLabel *title = new QLabel("<h2>LP light directions</h2>");
	content->addWidget(title);
	content->addSpacing(30);
	QHBoxLayout *filebox = new QHBoxLayout();
	QPushButton *load = new QPushButton("Load LP file...");
	connect(load, SIGNAL(clicked()), this, SLOT(loadLP()));
	load->setMaximumWidth(300);
	filebox->addWidget(load);
	QLabel *filename = new QLabel();
	filebox->addWidget(filename);

	content->addLayout(filebox);
	content->addStretch();
}

void LpFrame::init() {
}

void LpFrame::loadLP() {

	QFileDialog::getOpenFileName(this, "Load an LP file", )
}

SphereFrame::SphereFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *content = new QVBoxLayout(this);
	this->setLayout(content);

	QLabel *title = new QLabel("<h2>Sphere light directions</h2>");
	content->addWidget(title);
}

void SphereFrame::init() {
}

DomeFrame::DomeFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *content = new QVBoxLayout(this);
	this->setLayout(content);

	QLabel *title = new QLabel("<h2>Dome light directions</h2>");
	content->addWidget(title);
}

void DomeFrame::init() {
}

