#include "lightsframe.h"

#include <QVBoxLayout>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QLabel>

Card::Card(QString title, QString subtitle, QWidget *parent): QFrame(parent) {
	setFrameShape(QFrame::StyledPanel);
	QVBoxLayout *layout = new QVBoxLayout(this);

	QPushButton *button = new QPushButton(title);

	QObject::connect(button, SIGNAL(clicked()), this, SLOT(click()));
	button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	setStyleSheet("QPushButton { font-size: 24px; }");

	layout->addWidget(button);
	layout->addSpacing(30);
	QLabel *body = new QLabel(subtitle);
	body->setAlignment(Qt::AlignCenter);
	layout->addWidget(body);
}

LightsFrame::LightsFrame() {
	this->addWidget(createLightsChoice());
}
QFrame *LightsFrame::createLightsChoice() {
	QFrame *lights_choice = new QFrame();
	lights_choice->setObjectName("lights_choice");

	QVBoxLayout *content = new QVBoxLayout(lights_choice);

	content->addStretch(1);
	QHBoxLayout *buttons = new QHBoxLayout();
	buttons->setSpacing(30);

	content->addLayout(buttons, 0);

	buttons->addStretch(1);

	Card *lp = new Card("LP", "<p>Load a file (.lp) containging the light directions.</p>");

	buttons->addWidget(lp, 1);

	QPushButton *sphere =new QPushButton("Sphere:\n identify reflective spheres");
	connect(sphere, SIGNAL(clicked()), this, SLOT(showSphere()));
	sphere->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	buttons->addWidget(sphere, 0);

	QPushButton *dome = new QPushButton("Dome:\n select a preconfigured dome");
	connect(dome, SIGNAL(clicked()), this, SLOT(showDome()));
	dome->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	buttons->addWidget(dome, 0);

	buttons->addStretch(1);

	content->addStretch(2);
	return lights_choice;
}

QFrame *LightsFrame::createLp() {
	QFrame *lp_frame = new QFrame();
	return lp_frame;
}

QFrame *LightsFrame::createSphere() {
	QFrame *sphere_frame = new QFrame();
	return sphere_frame;
}

QFrame *LightsFrame::createDome() {
	QFrame *dome_frame = new QFrame();
	return dome_frame;
}

void LightsFrame::showLp() {
}

void LightsFrame::showSphere() {
}

void LightsFrame::showDome() {
}
