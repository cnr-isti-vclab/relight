#include "rtiplan.h"
#include "qlabelbutton.h"
#include "helpbutton.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QLineEdit>

RtiBasisRow::RtiBasisRow(QFrame *parent): QFrame(parent) {
	QHBoxLayout *layout = new QHBoxLayout(this);
	HelpLabel *label = new HelpLabel("Basis:", "rti/basis");
	label->setFixedWidth(200);
	layout->addWidget(label, 0, Qt::AlignLeft);

	QHBoxLayout *buttons = new QHBoxLayout;
	layout->addLayout(buttons, 1);
	buttons->setSpacing(20);

	buttons->addStretch(1);
	buttons->addWidget(new QLabelButton("PTM"), 0, Qt::AlignCenter);
	buttons->addWidget(new QLabelButton("HSH"), 0, Qt::AlignCenter);
	buttons->addWidget(new QLabelButton("RBF"), 0, Qt::AlignCenter);
	buttons->addWidget(new QLabelButton("BNL"), 0, Qt::AlignCenter);
	buttons->addStretch(1);
}

void RtiBasisRow::init(Rti::Type basis) {
	this->basis = basis;
}

RtiColorSpaceRow::RtiColorSpaceRow(QFrame *parent): QFrame(parent) {
	QHBoxLayout *layout = new QHBoxLayout(this);
	HelpLabel *label = new HelpLabel("Color space:","rti/colorspace");
	label->setFixedWidth(200);
	layout->addWidget(label, 0, Qt::AlignLeft);
	//layout->addWidget(new HelpLabel(""));
	layout->addWidget(new QLabelButton("RGB"));
	layout->addWidget(new QLabelButton("LRGB"));
	layout->addWidget(new QLabelButton("YCC"));
}

void RtiColorSpaceRow::init(Rti::ColorSpace colorspace) {
	this->colorspace = colorspace;
}


RtiPlanesRow::RtiPlanesRow(QFrame *parent): QFrame(parent) {
	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->addWidget(new HelpLabel("Planes:", "rti/planes"));
	layout->addWidget(new QLabelButton("12 coefficients"));
	layout->addWidget(new QLabelButton("15 coefficients"));
	layout->addWidget(new QLabelButton("18 coefficients"));
	layout->addWidget(new QLabelButton("21 coefficients"));
	layout->addWidget(new QLabelButton("24 coefficients"));
	layout->addWidget(new QLabelButton("27 coefficients"));
	layout->addWidget(new QLabelButton("0 chroma dedicated"));
	layout->addWidget(new QLabelButton("1 chroma dedicated"));
	layout->addWidget(new QLabelButton("2 chroma dedicated"));
}

void RtiPlanesRow::init(int nplanes, int nchroma) {
	this->nplanes = nplanes;
	this->nchroma = nchroma;
}

RtiFormatRow::RtiFormatRow(QFrame *parent): QFrame(parent) {
	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->addWidget(new HelpLabel("Format:", "rti/format"));
	layout->addWidget(new QLabelButton("Lossless (heavy!)"));
	layout->addWidget(new QLabelButton("JPEG"));
	layout->addWidget(new QLabel("Quality:"));
	layout->addWidget(new QSpinBox);
	layout->addWidget(new QLabel("Filename:"));
	layout->addWidget(new QLineEdit);
	layout->addWidget(new QPushButton("..."));
	layout->addWidget(new QPushButton("Export"));
}

void RtiFormatRow::init(RtiTask::Steps format, int quality) {
	this->format = format;
	this->quality = quality;
}

RtiPlan::RtiPlan(QWidget *parent): QFrame(parent) {
	QVBoxLayout *layout = new QVBoxLayout(this);

	basis_row = new RtiBasisRow(this);
	layout->addWidget(basis_row);
	connect(basis_row, &RtiBasisRow::basisChanged, this, &RtiPlan::basisChanged);

	colorspace_row = new RtiColorSpaceRow(this);
	layout->addWidget(colorspace_row);
	connect(colorspace_row, &RtiColorSpaceRow::colorspaceChanged, this, &RtiPlan::colorspaceChanged);

	planes_row = new RtiPlanesRow(this);
	layout->addWidget(planes_row);
	connect(planes_row, &RtiPlanesRow::nplanesChanged, this, &RtiPlan::nplanesChanged);

	format_row = new RtiFormatRow(this);
	layout->addWidget(format_row);
	connect(format_row, &RtiFormatRow::formatChanged, this, &RtiPlan::formatChanged);

}

void RtiPlan::basisChanged(Rti::Type basis) {
	//when basis is changed we try to change the other values as little as possible.
	//if the new basis is not compatible with the current color space we change it to the default one

}

void RtiPlan::colorspaceChanged(Rti::ColorSpace colorspace) {
	//when colorspace is changed we try to change the other values as little as possible.
	//if the new colorspace is not compatible with the current basis we change it to the default one

}

void RtiPlan::nplanesChanged(int nplanes, int nchroma) {
	//when nplanes is changed we try to change the other values as little as possible.
	//if the new nplanes is not compatible with the current basis we change it to the default one

}

void RtiPlan::formatChanged(RtiTask::Steps format) {
	//when format is changed we try to change the other values as little as possible.
	//if the new format is not compatible with the current basis we change it to the default one

}
