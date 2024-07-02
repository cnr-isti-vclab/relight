#include "rtiplan.h"
#include "qlabelbutton.h"
#include "helpbutton.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QCheckBox>
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

	auto *ptm = new QLabelButton("PTM", "Polynomial Texture Map");
	auto *hsh = new QLabelButton("HSH", "HemiSpherical Harmonics");
	auto *rbf = new QLabelButton("RBF", "Radial Basis Functions");
	auto *bnl = new QLabelButton("BNL", "Bilinear interplation");

	buttons->addWidget(ptm, 0, Qt::AlignCenter);
	buttons->addWidget(hsh, 0, Qt::AlignCenter);
	buttons->addWidget(rbf, 0, Qt::AlignCenter);
	buttons->addWidget(bnl, 0, Qt::AlignCenter);

	connect(ptm, &QAbstractButton::clicked, this, [this](){emit basisChanged(Rti::PTM);});
	connect(hsh, &QAbstractButton::clicked, this, [this](){emit basisChanged(Rti::HSH);});
	connect(rbf, &QAbstractButton::clicked, this, [this](){emit basisChanged(Rti::RBF);});
	connect(bnl, &QAbstractButton::clicked, this, [this](){emit basisChanged(Rti::BILINEAR);});

	QButtonGroup *group = new QButtonGroup(this);

	group->addButton(ptm);
	group->addButton(hsh);
	group->addButton(rbf);
	group->addButton(bnl);
}

void RtiBasisRow::init(Rti::Type basis) {
	this->basis = basis;
}

RtiColorSpaceRow::RtiColorSpaceRow(QFrame *parent): RtiPlanRow(parent) {

	label->label->setText("Colorspace:");
	label->help->setId("rti/colorspace");

	rgb  = new QLabelButton("RGB", "Standard");
	lrgb = new QLabelButton("LRGB", "Albedo * Luminance.");
	mrgb = new QLabelButton("MRGB", "Standard");
	ycc  = new QLabelButton("YCC", "Dedicated chroma coefficients.");

	buttons->addWidget(rgb, 0, Qt::AlignLeft);
	buttons->addWidget(lrgb, 0, Qt::AlignRight);
	buttons->addWidget(mrgb, 0, Qt::AlignLeft);
	buttons->addWidget(ycc, 0, Qt::AlignRight);

	connect(rgb,  &QAbstractButton::clicked, this, [this](){emit colorspaceChanged(Rti::RGB);});
	connect(lrgb, &QAbstractButton::clicked, this, [this](){emit colorspaceChanged(Rti::LRGB);});
	connect(mrgb, &QAbstractButton::clicked, this, [this](){emit colorspaceChanged(Rti::MRGB);});
	connect(ycc,  &QAbstractButton::clicked, this, [this](){emit colorspaceChanged(Rti::YCC);});

	QButtonGroup *group = new QButtonGroup(this);
	group->addButton(rgb);
	group->addButton(lrgb);
	group->addButton(mrgb);
	group->addButton(ycc);
}

void RtiColorSpaceRow::init(Rti::Type basis, Rti::ColorSpace colorspace) {
	bool pca = basis == Rti::RBF || basis == Rti::BILINEAR;

	rgb->setVisible(!pca);
	lrgb->setVisible(!pca);
	mrgb->setVisible(pca);
	ycc->setVisible(pca);
	this->colorspace = colorspace;
}


RtiPlanesRow::RtiPlanesRow(QFrame *parent): RtiPlanRow(parent) {

	label->label->setText("Planes:");
	label->help->setId("rti/planes");


	buttons->addWidget(new QLabel("Total number of images:"));
	nplanesbox = new QComboBox;
	nplanesbox->setFixedWidth(100);
	int nimages[] = { 3, 4, 5, 6, 7, 8, 9 };
	for(int i = 0; i < 7; i++) {
		nplanesbox->addItem(QString::number(nimages[i]));
	}	
	buttons->addWidget(nplanesbox);

	buttons->addStretch(1);
	buttons->addWidget(new QLabel("Number of dedicated chroma images:"));
	nchromabox = new QComboBox;
	nchromabox->setFixedWidth(100);
	int nchroma[] = { 1, 2, 3 };
	for(int i = 0; i < 3; i++) {
		nchromabox->addItem(QString::number(nchroma[i]));
	}
	buttons->addWidget(nchromabox);
}

void RtiPlanesRow::init(Rti::ColorSpace colorspace, int nplanes, int nchroma) {
	nplanesbox->setCurrentIndex(nplanes/3 - 3);
	nchromabox->setCurrentIndex(nchroma -1);

	this->nplanes = nplanes;
	this->nchroma = nchroma;
}

RtiFormatRow::RtiFormatRow(QFrame *parent): RtiPlanRow(parent) {
	label->label->setText("Format:");
	label->help->setId("rti/format");


	rti = new QLabelButton("Legacy", ".rti, .ptm");
	web = new QLabelButton("Web", ".json, .jpg");
	iip = new QLabelButton("IIP", ".tiff");

	buttons->addWidget(rti, Qt::AlignLeft);
	buttons->addWidget(web, Qt::AlignLeft);
	buttons->addWidget(iip, Qt::AlignLeft);

	QButtonGroup *group = new QButtonGroup(this);
	group->addButton(rti);
	group->addButton(web);
	group->addButton(iip);

}

RtiQualityRow::RtiQualityRow(QFrame *parent): RtiPlanRow(parent) {
	label->label->setText("Image quality:");
	label->help->setId("rti/quality");

	buttons->addWidget(new QCheckBox(" Lossless"), Qt::AlignLeft);

	QLabel *qualitylabel = new QLabel("Quality:");
	qualitylabel->setMaximumWidth(200);
	buttons->addWidget(qualitylabel, Qt::AlignRight);

	qualitybox = new QSpinBox;
	qualitybox->setMaximumWidth(200);
	qualitybox->setMinimum(75);
	qualitybox->setMaximum(100);
	qualitybox->setValue(quality);
	buttons->addWidget(qualitybox, Qt::AlignRight);


	/*buttons->addWidget(new QLabel("Filename:"));
	buttons->addWidget(new QLineEdit);
	buttons->addWidget(new QPushButton("..."));
	buttons->addWidget(new QPushButton("Export")); */
}
void RtiQualityRow::init(int quality) {

}

RtiWebLayoutRow::RtiWebLayoutRow(QFrame *parent) {
	label->label->setText("Web layout:");
	label->help->setId("rti/web_layout");

}
void RtiWebLayoutRow::init(RtiParameters::WebLayout layout) {

} //0 stands for lossless.


void RtiFormatRow::init(RtiParameters::Format format) {
	this->format = format;
}

RtiPlan::RtiPlan(QWidget *parent): QFrame(parent) {
	QVBoxLayout *layout = new QVBoxLayout(this);

	basis_row = new RtiBasisRow(this);
	colorspace_row = new RtiColorSpaceRow(this);
	planes_row = new RtiPlanesRow(this);
	format_row = new RtiFormatRow(this);
	quality_row = new RtiQualityRow(this);

	layout->addWidget(basis_row);
	layout->addWidget(colorspace_row);
	layout->addWidget(planes_row);
	layout->addWidget(format_row);
	layout->addWidget(quality_row);
	layout->addStretch();

	connect(basis_row, &RtiBasisRow::basisChanged, this, &RtiPlan::basisChanged);
	connect(colorspace_row, &RtiColorSpaceRow::colorspaceChanged, this, &RtiPlan::colorspaceChanged);
	connect(planes_row, &RtiPlanesRow::nplanesChanged, this, &RtiPlan::nplanesChanged);
	connect(format_row, &RtiFormatRow::formatChanged, this, &RtiPlan::formatChanged);
	connect(quality_row, &RtiQualityRow::qualityChanged, this, &RtiPlan::qualityChanged);
}


void RtiPlan::basisChanged(Rti::Type basis) {
	//when basis is changed we try to change the other values as little as possible.
	//if the new basis is not compatible with the current color space we change it to the default one
	//colorspace:
	parameters.basis = basis;
	bool pca = basis == Rti::RBF || basis == Rti::BILINEAR;

	auto &colorspace = parameters.colorspace;
	if(pca) {
		if(colorspace != Rti::RGB && colorspace != Rti::LRGB)
			colorspace = Rti::RGB;

	} else {
		if(colorspace != Rti::MRGB && colorspace != Rti::YCC)
			colorspace = Rti::MRGB;
	}
	colorspace_row->init(basis, colorspace);

	auto &nplanes = parameters.nplanes;
	auto &nchroma = parameters.nchroma;
	switch(basis) {
	case Rti::PTM: nplanes = 18; nchroma = 0; break;
	case Rti::HSH: if(nplanes != 12 && nplanes != 27) nplanes = 27; nchroma = 0; break;
	case Rti::RBF:
	case Rti::BILINEAR:
		if(colorspace != Rti::YCC) nchroma = 0;
	}

	planes_row->init(colorspace, nplanes, nchroma);


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

void RtiPlan::qualityChanged(int quality) {


}

void RtiPlan::layoutChanged(RtiParameters::WebLayout layout) {

}

