#include "rtiplan.h"
#include "qlabelbutton.h"
#include "helpbutton.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QLineEdit>
#include <QButtonGroup>
#include <QComboBox>

RtiPlanRow::RtiPlanRow(QFrame *parent): QFrame(parent) {
	QHBoxLayout *layout = new QHBoxLayout(this);

	label = new HelpLabel("", "");
	label->setFixedWidth(200);
	layout->addWidget(label, 0, Qt::AlignLeft);
	//layout->setSpacing(20);

	layout->addStretch(1);

	QFrame *buttonsFrame = new QFrame;
	buttonsFrame->setMinimumWidth(860);
	buttonsFrame->setFrameStyle(QFrame::Box);

	layout->addWidget(buttonsFrame);

	buttons = new QHBoxLayout(buttonsFrame);

	layout->addStretch(1);
}


RtiBasisRow::RtiBasisRow(QFrame *parent): RtiPlanRow(parent) {

	label->label->setText("Basis:");
	label->help->setId("rti/basis");

	QButtonGroup *group = new QButtonGroup(this);

	auto *ptm = new QLabelButton("PTM", "Polynomial Texture Map");
	group->addButton(ptm);
	buttons->addWidget(ptm, 0, Qt::AlignCenter);
	connect(ptm, &QAbstractButton::clicked, this, [this](){emit basisChanged(Rti::PTM);});

	auto *hsh = new QLabelButton("HSH", "HemiSpherical Harmonics");
	group->addButton(hsh);
	buttons->addWidget(hsh, 0, Qt::AlignCenter);
	connect(hsh, &QAbstractButton::clicked, this, [this](){emit basisChanged(Rti::HSH);});

	auto *rbf = new QLabelButton("RBF", "Radial Basis Functions");
	group->addButton(rbf);
	buttons->addWidget(rbf, 0, Qt::AlignCenter);
	connect(rbf, &QAbstractButton::clicked, this, [this](){emit basisChanged(Rti::RBF);});

	auto *bnl = new QLabelButton("BNL", "Bilinear interplation");
	group->addButton(bnl);
	buttons->addWidget(bnl, 0, Qt::AlignCenter);
	connect(bnl, &QAbstractButton::clicked, this, [this](){emit basisChanged(Rti::BILINEAR);});


}

void RtiBasisRow::init(Rti::Type basis) {
	this->basis = basis;
}

RtiColorSpaceRow::RtiColorSpaceRow(QFrame *parent): RtiPlanRow(parent) {

	label->label->setText("Colorspace:");
	label->help->setId("rti/colorspace");


	QButtonGroup *group = new QButtonGroup(this);
	auto *rgb = new QLabelButton("RGB", "Standard");
	group->addButton(rgb);
	buttons->addWidget(rgb, 0, Qt::AlignLeft);
	connect(rgb, &QAbstractButton::clicked, this, [this](){emit colorspaceChanged(Rti::RGB);});

	auto *lrgb = new QLabelButton("LRGB", "Albedo * Luminance.");
	group->addButton(lrgb);
	buttons->addWidget(lrgb, 0, Qt::AlignCenter);
	connect(lrgb, &QAbstractButton::clicked, this, [this](){emit colorspaceChanged(Rti::LRGB);});

	auto *ycc = new QLabelButton("YCC", "Dedicated chroma coefficients.");
	group->addButton(ycc);
	buttons->addWidget(ycc, 0, Qt::AlignRight);
	connect(ycc, &QAbstractButton::clicked, this, [this](){emit colorspaceChanged(Rti::YCC);});

}

void RtiColorSpaceRow::init(Rti::ColorSpace colorspace) {
	this->colorspace = colorspace;
}


RtiPlanesRow::RtiPlanesRow(QFrame *parent): RtiPlanRow(parent) {

	label->label->setText("Planes:");
	label->help->setId("rti/planes");


	buttons->addWidget(new QLabel("Total number of images:"));
	auto *total_images = new QComboBox;
	total_images->setFixedWidth(100);
	int nimages[] = { 3, 4, 5, 6, 7, 8, 9 };
	for(int i = 0; i < 7; i++) {
		total_images->addItem(QString::number(nimages[i]));
	}	
	buttons->addWidget(total_images);

	buttons->addStretch(1);
	buttons->addWidget(new QLabel("Number of dedicated chroma images:"));
	auto *chroma = new QComboBox;
	chroma->setFixedWidth(100);
	int nchroma[] = { 1, 2, 3 };
	for(int i = 0; i < 3; i++) {
		chroma->addItem(QString::number(nchroma[i]));
	}
	buttons->addWidget(chroma);
}

void RtiPlanesRow::init(int nplanes, int nchroma) {
	this->nplanes = nplanes;
	this->nchroma = nchroma;
}

RtiFormatRow::RtiFormatRow(QFrame *parent): RtiPlanRow(parent) {
	label->label->setText("Format:");
	label->help->setId("rti/format");

	rti = new QLabelButton("legacy (.rti, .ptm)");
	buttons->addWidget(rti, Qt::AlignLeft);

	web = new QLabelButton("web (json + jpeg");
	buttons->addWidget(web, Qt::AlignLeft);

	iip = new QLabelButton("iip (tiff)");
	buttons->addWidget(iip, Qt::AlignLeft);

/*	buttons->addWidget(new QLabelButton("Lossless (heavy!)"));
	buttons->addWidget(new QLabelButton("JPEG"));
	buttons->addWidget(new QLabel("Quality:"));
	buttons->addWidget(new QSpinBox);
	buttons->addWidget(new QLabel("Filename:"));
	buttons->addWidget(new QLineEdit);
	buttons->addWidget(new QPushButton("..."));
	buttons->addWidget(new QPushButton("Export")); */
}

void RtiFormatRow::init(RtiParameters::Format format) {
	this->format = format;
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

	layout->addStretch();

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

void RtiPlan::formatChanged(RtiParameters::Format format) {
	//when format is changed we try to change the other values as little as possible.
	//if the new format is not compatible with the current basis we change it to the default one

}
