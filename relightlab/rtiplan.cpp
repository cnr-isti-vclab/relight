#include "rtiplan.h"
#include "rtiframe.h"
#include "qlabelbutton.h"
#include "helpbutton.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QButtonGroup>
#include <QComboBox>

RtiPlanRow::RtiPlanRow(RtiParameters &param, QFrame *parent): QFrame(parent), parameters(param) {
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


RtiBasisRow::RtiBasisRow(RtiParameters &parameters, QFrame *parent): RtiPlanRow(parameters, parent) {
	label->label->setText("Basis:");
	label->help->setId("rti/basis");

	ptm = new QLabelButton("PTM", "Polynomial Texture Map");
	hsh = new QLabelButton("HSH", "HemiSpherical Harmonics");
	rbf = new QLabelButton("RBF", "Radial Basis Functions");
	bln = new QLabelButton("BNL", "Bilinear interplation");

	buttons->addWidget(ptm, 0, Qt::AlignCenter);
	buttons->addWidget(hsh, 0, Qt::AlignCenter);
	buttons->addWidget(rbf, 0, Qt::AlignCenter);
	buttons->addWidget(bln, 0, Qt::AlignCenter);

	connect(ptm, &QAbstractButton::clicked, this, [this](){ setBasis(Rti::PTM, true); });
	connect(hsh, &QAbstractButton::clicked, this, [this](){ setBasis(Rti::HSH, true); });
	connect(rbf, &QAbstractButton::clicked, this, [this](){ setBasis(Rti::RBF, true); });
	connect(bln, &QAbstractButton::clicked, this, [this](){ setBasis(Rti::BILINEAR, true); });

	QButtonGroup *group = new QButtonGroup(this);

	group->addButton(ptm);
	group->addButton(hsh);
	group->addButton(rbf);
	group->addButton(bln);
	setBasis(parameters.basis);
}

void RtiBasisRow::setBasis(Rti::Type basis, bool emitting) {
	parameters.basis = basis;

	if(emitting) {
		emit basisChanged();
		return;
	}

	switch(basis) {
		case Rti::PTM: ptm->setChecked(true); break;
		case Rti::HSH: hsh->setChecked(true); break;
		case Rti::RBF: rbf->setChecked(true); break;
		case Rti::BILINEAR: bln->setChecked(true); break;
		default: break;
	}
}

RtiColorSpaceRow::RtiColorSpaceRow(RtiParameters &parameters, QFrame *parent): RtiPlanRow(parameters, parent) {

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

	connect(rgb,  &QAbstractButton::clicked, this, [this](){ setColorspace(Rti::RGB, true);  });
	connect(lrgb, &QAbstractButton::clicked, this, [this](){ setColorspace(Rti::LRGB, true); });
	connect(mrgb, &QAbstractButton::clicked, this, [this](){ setColorspace(Rti::MRGB, true); });
	connect(ycc,  &QAbstractButton::clicked, this, [this](){ setColorspace(Rti::YCC, true);  });

	QButtonGroup *group = new QButtonGroup(this);
	group->addButton(rgb);
	group->addButton(lrgb);
	group->addButton(mrgb);
	group->addButton(ycc);

	setColorspace(parameters.colorspace);
}


void RtiColorSpaceRow::setColorspace(Rti::ColorSpace colorspace, bool emitting) {
	parameters.colorspace = colorspace;

	bool pca = parameters.basis == Rti::RBF || parameters.basis == Rti::BILINEAR;

	rgb->setEnabled(!pca);
	lrgb->setEnabled(!pca);
	mrgb->setEnabled(pca);
	ycc->setEnabled(pca);

	if(emitting) {
		emit colorspaceChanged();
		return;
	}
	switch(colorspace) {
		case Rti::RGB:  rgb->setChecked(true); break;
		case Rti::LRGB: lrgb->setChecked(true); break;
		case Rti::MRGB: mrgb->setChecked(true); break;
		case Rti::YCC:  ycc->setChecked(true); break;
	}
}


RtiPlanesRow::RtiPlanesRow(RtiParameters &parameters, QFrame *parent): RtiPlanRow(parameters, parent) {

	label->label->setText("Planes:");
	label->help->setId("rti/planes");


	buttons->addWidget(new QLabel("Total number of images:"));
	nplanesbox = new QComboBox;
	nplanesbox->setFixedWidth(100);
	for(int i = 0; i < 7; i++) {
		nplanesbox->addItem(QString::number(nimages[i]));
	}	
	connect(nplanesbox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int n) { setNPlanes(nimages[n]*3, true); });
	buttons->addWidget(nplanesbox);

	buttons->addStretch(1);
	buttons->addWidget(new QLabel("Number of dedicated chroma images:"));
	nchromabox = new QComboBox;
	nchromabox->setFixedWidth(100);

	for(int i = 0; i < 3; i++) {
		nchromabox->addItem(QString::number(nchromas[i]));
	}
	connect(nchromabox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int n) { setNChroma(nchromas[n], true); });
	buttons->addWidget(nchromabox);

	setNPlanes(parameters.nplanes);
	setNChroma(parameters.nchroma);
}

void RtiPlanesRow::setNPlanes(int nplanes, bool emitting) {
	nchromabox->setEnabled(parameters.colorspace == Rti::YCC);

	parameters.nplanes = nplanes;
	if(emitting) {
		emit nplanesChanged();
		return;
	}
	for(int i = 0; i < 7; i++) {
		if(nimages[i] == nplanes/3)
			nplanesbox->setCurrentIndex(i);
	}
}

void RtiPlanesRow::setNChroma(int nchroma, bool emitting) {
	nchromabox->setEnabled(parameters.colorspace == Rti::YCC);

	parameters.nchroma = nchroma;
	if(emitting) {
		emit nplanesChanged();
		return;
	}

	for(int i = 0; i < 3; i++) {
		if(nchromas[i] == nchroma)
			nchromabox->setCurrentIndex(i);
	}
}


RtiFormatRow::RtiFormatRow(RtiParameters &parameters, QFrame *parent): RtiPlanRow(parameters, parent) {
	label->label->setText("Format:");
	label->help->setId("rti/format");


	rti = new QLabelButton("Legacy", ".rti, .ptm");
	web = new QLabelButton("Web", ".json, .jpg");
	iip = new QLabelButton("IIP", ".tiff");

	buttons->addWidget(rti, Qt::AlignLeft);
	buttons->addWidget(web, Qt::AlignLeft);
	buttons->addWidget(iip, Qt::AlignLeft);

	connect(rti, &QAbstractButton::clicked, [this]() { setFormat(RtiParameters::RTI, true); });
	connect(web, &QAbstractButton::clicked, [this]() { setFormat(RtiParameters::WEB, true); });
	connect(iip, &QAbstractButton::clicked, [this]() { setFormat(RtiParameters::IIP, true); });

	QButtonGroup *group = new QButtonGroup(this);
	group->addButton(rti);
	group->addButton(web);
	group->addButton(iip);

	allowLegacy(parameters.basis == Rti::PTM || parameters.basis == Rti::HSH);
	setFormat(parameters.format);
}

void RtiFormatRow::allowLegacy(bool legacy) {
	rti->setEnabled(legacy);
}

void RtiFormatRow::setFormat(RtiParameters::Format format, bool emitting) {
	parameters.format = format;

	if(emitting) {
		emit formatChanged();
		return;
	}
	switch(format) {
		case RtiParameters::RTI: rti->setChecked(true); break;
		case RtiParameters::WEB: web->setChecked(true); break;
		case RtiParameters::IIP: iip->setChecked(true); break;
	}

}

RtiQualityRow::RtiQualityRow(RtiParameters &parameters, QFrame *parent): RtiPlanRow(parameters, parent) {
	label->label->setText("Image quality:");
	label->help->setId("rti/quality");

	buttons->addWidget(losslessbox = new QCheckBox(" Lossless"), Qt::AlignLeft);

	QLabel *qualitylabel = new QLabel("Quality:");
	qualitylabel->setMaximumWidth(200);
	buttons->addWidget(qualitylabel, Qt::AlignRight);

	qualitybox = new QSpinBox;
	qualitybox->setMaximumWidth(200);
	qualitybox->setMinimum(75);
	qualitybox->setMaximum(100);
	qualitybox->setValue(parameters.quality);
	buttons->addWidget(qualitybox, Qt::AlignRight);
}

void RtiQualityRow::setQuality(int quality, bool emitting) {
	parameters.quality = quality;
	if(quality == 0)
		losslessbox->setChecked(true);

	if(emitting)
		emit qualityChanged();
	else
		qualitybox->setValue(quality);
}
void RtiQualityRow::allowLossless(bool allow) {
	losslessbox->setEnabled(allow);
}

RtiWebLayoutRow::RtiWebLayoutRow(RtiParameters &parameters, QFrame *parent): RtiPlanRow(parameters, parent) {
	label->label->setText("Web layout:");
	label->help->setId("rti/web_layout");

	image    = new QLabelButton("Images", "");
	deepzoom = new QLabelButton("Deepzoom", "Pyramidal, lot's of files.");
	tarzoom  = new QLabelButton("Tarzoom", "Pyramidal, few files but 206");
	itarzoom = new QLabelButton("ITarzoom", "Pyramidal, few files and requests but 206");

	buttons->addWidget(image);
	buttons->addWidget(deepzoom);
	buttons->addWidget(tarzoom);
	buttons->addWidget(itarzoom);

	setWebLayout(parameters.web_layout);

	connect(image,    &QAbstractButton::clicked, [this]() { setWebLayout(RtiParameters::PLAIN); });
	connect(deepzoom, &QAbstractButton::clicked, [this]() { setWebLayout(RtiParameters::DEEPZOOM); });
	connect(tarzoom,  &QAbstractButton::clicked, [this]() { setWebLayout(RtiParameters::TARZOOM); });
	connect(itarzoom, &QAbstractButton::clicked, [this]() { setWebLayout(RtiParameters::ITARZOOM); });

	QButtonGroup *group = new QButtonGroup(this);
	group->addButton(image);
	group->addButton(deepzoom);
	group->addButton(tarzoom);
	group->addButton(itarzoom);
}

void RtiWebLayoutRow::setWebLayout(RtiParameters::WebLayout layout, bool emitting) {
	parameters.web_layout = layout;

	if(emitting) {
		emit layoutChanged();
		return;
	}

	image->setChecked(layout == RtiParameters::PLAIN);
	deepzoom->setChecked(layout == RtiParameters::DEEPZOOM);
	tarzoom->setChecked(layout == RtiParameters::TARZOOM);
	itarzoom->setChecked(layout == RtiParameters::ITARZOOM);
}



RtiPlan::RtiPlan(QWidget *parent): QFrame(parent) {
	QVBoxLayout *layout = new QVBoxLayout(this);

	basis_row = new RtiBasisRow(parameters, this);
	colorspace_row = new RtiColorSpaceRow(parameters, this);
	planes_row = new RtiPlanesRow(parameters, this);
	format_row = new RtiFormatRow(parameters, this);
	quality_row = new RtiQualityRow(parameters, this);
	layout_row = new RtiWebLayoutRow(parameters, this);


	layout->addWidget(basis_row);
	layout->addWidget(colorspace_row);
	layout->addWidget(planes_row);
	layout->addWidget(format_row);
	layout->addWidget(quality_row);
	layout->addWidget(layout_row);

	QHBoxLayout *save_row = new QHBoxLayout;
	QPushButton *save = new QPushButton("Export RTI...", this);
	save->setProperty("class", "large");
	save_row->addWidget(save);
	RtiFrame *parent_frame = dynamic_cast<RtiFrame *>(parent);
	connect(save, &QPushButton::clicked, [this, parent_frame]() { return parent_frame->exportRti(this->parameters); });

	layout->addLayout(save_row);
	layout->addStretch();

	connect(basis_row, &RtiBasisRow::basisChanged, this, &RtiPlan::basisChanged);
	connect(colorspace_row, &RtiColorSpaceRow::colorspaceChanged, this, &RtiPlan::colorspaceChanged);
	connect(planes_row, &RtiPlanesRow::nplanesChanged, this, &RtiPlan::nplanesChanged);
	connect(format_row, &RtiFormatRow::formatChanged, this, &RtiPlan::formatChanged);
	connect(layout_row, &RtiWebLayoutRow::layoutChanged, this, &RtiPlan::layoutChanged);
	connect(quality_row, &RtiQualityRow::qualityChanged, this, &RtiPlan::qualityChanged);
}


void RtiPlan::basisChanged() {
	//when basis is changed we try to change the other values as little as possible.
	//if the new basis is not compatible with the current color space we change it to the default one
	//colorspace:
	auto &basis = parameters.basis;
	bool pca = basis == Rti::RBF || basis == Rti::BILINEAR;

	// COLORSPACE

	auto &colorspace = parameters.colorspace;
	if(!pca) {
		if(colorspace != Rti::RGB && colorspace != Rti::LRGB)
			colorspace = Rti::RGB;

	} else {
		if(colorspace != Rti::MRGB && colorspace != Rti::YCC)
			colorspace = Rti::MRGB;
	}
	colorspace_row->setColorspace(colorspace);

	// PLANES

	auto &nplanes = parameters.nplanes;
	auto &nchroma = parameters.nchroma;

	switch(basis) {
	case Rti::PTM: nplanes = 18; nchroma = 0; break;
	case Rti::HSH: if(nplanes != 12 && nplanes != 27) nplanes = 27; nchroma = 0; break;
	case Rti::RBF:
	case Rti::BILINEAR:
		if(colorspace != Rti::YCC) nchroma = 0;
	}

	planes_row->setNPlanes(nplanes);
	planes_row->setNChroma(nchroma);

	//FORMAT
	format_row->allowLegacy(!pca);

	if(pca && parameters.format == RtiParameters::RTI) {
		format_row->setFormat(RtiParameters::WEB); //emit and cascade update other rows.
	}
}

void RtiPlan::colorspaceChanged() {
	planes_row->setNChroma(parameters.nchroma);

	bool pca = parameters.basis == Rti::RBF || parameters.basis == Rti::BILINEAR;
	if(pca && parameters.format == RtiParameters::RTI) {
		format_row->setFormat(RtiParameters::WEB);
	}
}

void RtiPlan::nplanesChanged() {
	//actually nothing happens!
}

void RtiPlan::formatChanged() {
	bool legacy = quality_row->parameters.format == RtiParameters::RTI;
	//only RTI allows for lossless.
	quality_row->allowLossless(legacy);
	layout_row->setEnabled(quality_row->parameters.format == RtiParameters::WEB);
}

void RtiPlan::qualityChanged() {
}

void RtiPlan::layoutChanged() {
}

