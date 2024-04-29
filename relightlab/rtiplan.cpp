#include "rtiplan.h"
#include "qlabelbutton.h"
#include "helpbutton.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QLineEdit>
#include <QButtonGroup>
#include <QComboBox>

RtiBasisRow::RtiBasisRow(QFrame *parent): QFrame(parent) {
	QHBoxLayout *layout = new QHBoxLayout(this);
	HelpLabel *label = new HelpLabel("Basis:", "rti/basis");
	label->setFixedWidth(200);
	layout->addWidget(label, 0, Qt::AlignLeft);

	layout->addStretch(1);

	QButtonGroup *group = new QButtonGroup(this);

	auto *ptm = new QLabelButton("PTM", "Polynomial Texture Map\nThis is a test");
	group->addButton(ptm);
	layout->addWidget(ptm, 0, Qt::AlignCenter);
	connect(ptm, &QAbstractButton::clicked, this, [this](){emit basisChanged(Rti::PTM);});

	auto *hsh = new QLabelButton("HSH", "HemiSpherical Harmonics");
	group->addButton(hsh);
	layout->addWidget(hsh, 0, Qt::AlignCenter);
	connect(hsh, &QAbstractButton::clicked, this, [this](){emit basisChanged(Rti::HSH);});

	auto *rbf = new QLabelButton("RBF", "Radial Basis Functions + PCA");
	group->addButton(rbf);
	layout->addWidget(rbf, 0, Qt::AlignCenter);
	connect(rbf, &QAbstractButton::clicked, this, [this](){emit basisChanged(Rti::RBF);});

	auto *bnl = new QLabelButton("BNL", "Bilinear interplation  + PCA");
	group->addButton(bnl);
	layout->addWidget(bnl, 0, Qt::AlignCenter);
	connect(bnl, &QAbstractButton::clicked, this, [this](){emit basisChanged(Rti::BILINEAR);});

	layout->addStretch(1);
}

void RtiBasisRow::init(Rti::Type basis) {
	this->basis = basis;
}

RtiColorSpaceRow::RtiColorSpaceRow(QFrame *parent): QFrame(parent) {
	QHBoxLayout *layout = new QHBoxLayout(this);
	HelpLabel *label = new HelpLabel("Color space:","rti/colorspace");
	label->setFixedWidth(200);
	layout->addWidget(label, 0, Qt::AlignLeft);
	layout->setSpacing(20);


	layout->addStretch(1);


	QButtonGroup *group = new QButtonGroup(this);
	auto *rgb = new QLabelButton("RGB", "Standard");
	group->addButton(rgb);
	layout->addWidget(rgb, 0, Qt::AlignCenter);
	connect(rgb, &QAbstractButton::clicked, this, [this](){emit colorspaceChanged(Rti::RGB);});

	auto *lrgb = new QLabelButton("LRGB", "Albedo * Luminance.");
	group->addButton(lrgb);
	layout->addWidget(lrgb, 0, Qt::AlignCenter);
	connect(lrgb, &QAbstractButton::clicked, this, [this](){emit colorspaceChanged(Rti::LRGB);});

	auto *ycc = new QLabelButton("YCC", "Dedicated chrooma coefficients.");
	group->addButton(ycc);
	layout->addWidget(ycc, 0, Qt::AlignCenter);
	connect(ycc, &QAbstractButton::clicked, this, [this](){emit colorspaceChanged(Rti::YCC);});

	layout->addStretch(1);

}

void RtiColorSpaceRow::init(Rti::ColorSpace colorspace) {
	this->colorspace = colorspace;
}


RtiPlanesRow::RtiPlanesRow(QFrame *parent): QFrame(parent) {
	QHBoxLayout *layout = new QHBoxLayout(this);
	HelpLabel *label = new HelpLabel("Planes:", "rti/planes");
	label->setFixedWidth(200);
	layout->addWidget(label, 0, Qt::AlignLeft);

	layout->addStretch(1);
	layout->addWidget(new QLabel("Total number of images:"));
	auto *total_images = new QComboBox;
	total_images->setFixedWidth(100);
	int nimages[] = { 3, 4, 5, 6, 7, 8, 9 };
	for(int i = 0; i < 7; i++) {
		total_images->addItem(QString::number(nimages[i]));
	}	
	layout->addWidget(total_images);

	layout->addStretch(1);
	layout->addWidget(new QLabel("Number of dedicated chroma images:"));
	auto *chroma = new QComboBox;
	chroma->setFixedWidth(100);
	int nchroma[] = { 1, 2, 3 };
	for(int i = 0; i < 3; i++) {
		chroma->addItem(QString::number(nchroma[i]));
	}
	layout->addWidget(chroma);

	layout->addStretch(1);

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

void RtiPlan::formatChanged(RtiTask::Steps format) {
	//when format is changed we try to change the other values as little as possible.
	//if the new format is not compatible with the current basis we change it to the default one

}
