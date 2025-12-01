#include "rtiplan.h"
#include "rtiframe.h"
#include "qlabelbutton.h"
#include "helpbutton.h"
#include "relightapp.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QButtonGroup>
#include <QComboBox>
#include <QStandardItemModel>
#include <QLineEdit>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>

#include <iostream>
using namespace std;
RtiPlanRow::RtiPlanRow(RtiParameters &param, QFrame *parent): PlanRow(parent), parameters(param) {
}


RtiBasisRow::RtiBasisRow(RtiParameters &parameters, QFrame *parent): RtiPlanRow(parameters, parent) {
	label->label->setText("Basis:");
	label->help->setId("rti/basis");

	buttons->addWidget(ptm = new QLabelButton("PTM", "Polynomial Texture Map"));
	buttons->addWidget(hsh = new QLabelButton("HSH", "HemiSpherical Harmonics"));
	buttons->addWidget(rbf = new QLabelButton("RBF", "Radial Basis Functions"));
	buttons->addWidget(bln = new QLabelButton("BLN", "Bilinear interplation"));

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

	buttons->addWidget(rgb  = new QLabelButton("RGB", "Standard"));
	buttons->addWidget(lrgb = new QLabelButton("LRGB", "Albedo * Luminance."));
	buttons->addWidget(mrgb = new QLabelButton("MRGB", "Standard"));
	buttons->addWidget(ycc  = new QLabelButton("YCC", "Dedicated chroma coefficients."));

	connect(rgb,  &QAbstractButton::clicked, this, [this](){ setColorspace(Rti::RGB, true);  });
	connect(lrgb, &QAbstractButton::clicked, this, [this](){ setColorspace(Rti::LRGB, true); });
	connect(mrgb, &QAbstractButton::clicked, this, [this](){ setColorspace(Rti::MRGB, true); });
	connect(ycc,  &QAbstractButton::clicked, this, [this](){ setColorspace(Rti::MYCC, true);  });

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
	case Rti::MYCC:  ycc->setChecked(true); break;
	default:
		break; //TODO check MYCC!
	}
}


RtiPlanesRow::RtiPlanesRow(RtiParameters &parameters, QFrame *parent): RtiPlanRow(parameters, parent) {

	label->label->setText("Planes:");
	label->help->setId("rti/planes");


	buttons->addWidget(new QLabel("Number of coefficients:"));
	nplanesbox = new QComboBox;
	nplanesbox->setFixedWidth(100);
	for(int i = 0; i < 8; i++) {
		nplanesbox->addItem(QString::number(nplanes[i]), QVariant(nplanes[i]));
	}
	connect(nplanesbox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int n) {
		setNPlanes(nplanes[n], true);
	});
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

void RtiPlanesRow::forceNPlanes(QList<int> n_planes) {
	QStandardItemModel *model =	  qobject_cast<QStandardItemModel *>(nplanesbox->model());
	Q_ASSERT(model != nullptr);
	for(int i = 0; i < model->rowCount(); i++) {
		QStandardItem *item = model->item(i);
		bool disabled = !n_planes.contains(nplanes[i]);
		item->setFlags(disabled ? item->flags() & ~Qt::ItemIsEnabled
								: item->flags() | Qt::ItemIsEnabled);
		if(!disabled)
			nplanesbox->setCurrentIndex(i);
	}
}


void RtiPlanesRow::forceNPlanes(int n_planes) {
	QList<int> l;
	l.push_back(n_planes);
	forceNPlanes(l);
	/*QStandardItemModel *model =	  qobject_cast<QStandardItemModel *>(nplanesbox->model());
	Q_ASSERT(model != nullptr);
	for(int i = 0; i < 7; i++) {
		QStandardItem *item = model->item(i);
		bool disabled = n_planes >= 0 && n_planes != nplanes[i];
		item->setFlags(disabled ? item->flags() & ~Qt::ItemIsEnabled
								: item->flags() | Qt::ItemIsEnabled);
	}*/
}

void RtiPlanesRow::setNPlanes(int nplanes, bool emitting) {
	nchromabox->setEnabled(parameters.colorspace == Rti::MYCC);

	parameters.nplanes = nplanes;
	if(emitting) {
		emit nplanesChanged();
		return;
	}
	for(int i = 0; i < nplanesbox->count(); i++) {
		if(nplanes == nplanesbox->itemData(i).toInt())
			nplanesbox->setCurrentIndex(i);
	}
}

void RtiPlanesRow::setNChroma(int nchroma, bool emitting) {
	nchromabox->setEnabled(parameters.colorspace == Rti::MYCC);

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

	buttons->addWidget(rti = new QLabelButton("Legacy", ".rti, .ptm"));
	buttons->addWidget(web = new QLabelButton("Web", ".json, .jpg"));
	buttons->addWidget(iip = new QLabelButton("IIP", ".tiff"));

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
	
	buttons->addSpacing(20);
	
	// Color profile info and controls
	profileLabel = new QLabel("Profile: Unknown");
	profileLabel->setMaximumWidth(250);
	profileLabel->setVisible(false);
	buttons->addWidget(profileLabel, Qt::AlignRight);
	
	buttons->addWidget(preserve = new QLabelButton("Preserve", "Keep input color profile"));
	buttons->addWidget(srgb = new QLabelButton("sRGB", "Convert to sRGB color space"));
	buttons->addWidget(displayp3 = new QLabelButton("Display P3", "Convert to Display P3 color space"));
	preserve->setVisible(false);
	srgb->setVisible(false);
	displayp3->setVisible(false);
	
	connect(preserve, &QAbstractButton::clicked, this, [this](){ setColorProfileMode(COLOR_PROFILE_PRESERVE, true); });
	connect(srgb, &QAbstractButton::clicked, this, [this](){ setColorProfileMode(COLOR_PROFILE_SRGB, true); });
	connect(displayp3, &QAbstractButton::clicked, this, [this](){ setColorProfileMode(COLOR_PROFILE_DISPLAY_P3, true); });
	
	QButtonGroup *group = new QButtonGroup(this);
	group->addButton(preserve);
	group->addButton(srgb);
	group->addButton(displayp3);
	
	setColorProfileMode(parameters.colorProfileMode);
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

void RtiQualityRow::setColorProfileMode(ColorProfileMode mode, bool emitting) {
	parameters.colorProfileMode = mode;

	if(emitting) {
		emit colorProfileModeChanged();
		return;
	}

	switch(mode) {
	case COLOR_PROFILE_PRESERVE: preserve->setChecked(true); break;
	case COLOR_PROFILE_SRGB: srgb->setChecked(true); break;
	case COLOR_PROFILE_DISPLAY_P3: displayp3->setChecked(true); break;
	}
}

void RtiQualityRow::updateProfileInfo(const QString &profileDesc, bool isSRGB, bool isDisplayP3) {
	QString text = "Profile: " + profileDesc;
	profileLabel->setText(text);
	
	// Show/hide profile controls based on whether profile exists
	bool hasProfile = (profileDesc != "No profile");
	profileLabel->setVisible(hasProfile);
	preserve->setVisible(hasProfile);
	srgb->setVisible(hasProfile);
	displayp3->setVisible(hasProfile);
	
	bool allowSRGB = hasProfile && !isSRGB;
	bool allowP3 = hasProfile && !isDisplayP3;
	srgb->setEnabled(allowSRGB);
	displayp3->setEnabled(allowP3);
	
	if(!hasProfile) {
		setColorProfileMode(COLOR_PROFILE_PRESERVE, false);
		preserve->setChecked(true);
		return;
	}

	switch(parameters.colorProfileMode) {
	case COLOR_PROFILE_SRGB:
		if(!allowSRGB) {
			setColorProfileMode(COLOR_PROFILE_PRESERVE, false);
			preserve->setChecked(true);
		}
		break;
	case COLOR_PROFILE_DISPLAY_P3:
		if(!allowP3) {
			setColorProfileMode(COLOR_PROFILE_PRESERVE, false);
			preserve->setChecked(true);
		}
		break;
	case COLOR_PROFILE_PRESERVE:
	default:
		preserve->setChecked(true);
		break;
	}
}

RtiWebLayoutRow::RtiWebLayoutRow(RtiParameters &parameters, QFrame *parent):
	RtiPlanRow(parameters, parent) {
	//reparent buttons to make space for export openlime viewer checkbox

	label->label->setText("Web layout:");
	label->help->setId("rti/web");

	image    = new QLabelButton("Images", "");
	deepzoom = new QLabelButton("Deepzoom", "Pyramidal, lot's of files.");
	tarzoom  = new QLabelButton("Tarzoom",  "Pyramidal, few files.");
	itarzoom = new QLabelButton("ITarzoom", "Pyramidal, one file.");

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

	QCheckBox *openlime = new QCheckBox("Add openlime viewer code.");
	openlime->setChecked(parameters.openlime);
	connect(openlime, &QCheckBox::stateChanged, [this](int state) { this->parameters.openlime = state > 0; });
	planLayout->addWidget(openlime);
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


RtiExportRow::RtiExportRow(RtiParameters &parameters, QFrame *parent): RtiPlanRow(parameters, parent) {
	label->label->setText("Directory/File:");
	label->help->setId("rti/export");

	path_edit = new QLineEdit;
	connect(path_edit, &QLineEdit::editingFinished,this, &RtiExportRow::verifyPath);
	buttons->addWidget(path_edit);
	QPushButton *path_button = new QPushButton("...");
	buttons->addWidget(path_button);
	connect(path_button, &QPushButton::clicked, this, &RtiExportRow::selectOutput);
}

void RtiExportRow::setPath(QString path, bool emitting) {
	path_edit->setText(path);
	parameters.path = path;
}

void RtiExportRow::verifyPath() {
	parameters.path = QString();
	QString path = path_edit->text();
	QDir path_dir(path);
	path_dir.cdUp();
	if(!path_dir.exists()) {
		QMessageBox::warning(this, "Invalid output path", "The specified path is not valid");
		return;
	}
	QString extension;
	if(parameters.format == RtiParameters::RTI) {
		extension = parameters.basis == Rti::HSH ? ".rti" : ".ptm";
	} else if(parameters.format == RtiParameters::IIP) {
		extension = ".tif";
	}
	if(!path.endsWith(extension)) {
		path += extension;
		path_edit->setText(path);
	}
	parameters.path = path;
}

void RtiExportRow::selectOutput() {
	//get folder if not legacy.
	QString output_parent = qRelightApp->lastOutputDir();

	QString output;
	if(parameters.format == RtiParameters::RTI) {
		QString extension;
		QString label;

		if(parameters.basis == Rti::HSH) {
			extension = ".rti";
			label = "RTI file (*.rti)";
		} else if(parameters.basis == Rti::PTM) {
			extension = ".ptm";
			label = "PTM file (*.ptm)";
		}
		output = QFileDialog::getSaveFileName(this, "Select a file name", output_parent, label);
		if(output.isNull()) return;

		if(!output.endsWith(extension))
			output += extension;

	} else {
		output = QFileDialog::getSaveFileName(this, "Select an output folder", output_parent);
		if(output.isNull()) return;
	}
	QDir output_parent_dir(output);
	output_parent_dir.cdUp();
	qRelightApp->setLastOutputDir(output_parent_dir.absolutePath());
	setPath(output);
}

void RtiExportRow::suggestPath() {
	QDir input = qRelightApp->project().dir;
	QString filename;

	if(parameters.format == RtiParameters::RTI) {
		filename = input.dirName() + (parameters.basis == Rti::PTM ? ".ptm" : ".rti");

	} else {

		switch(parameters.basis) {
		case Rti::PTM: filename = parameters.colorspace == Rti::RGB ? "ptm" : "lptm"; break;
		case Rti::HSH: filename = parameters.colorspace == Rti::RGB ? "hsh" : "lhsh"; break;
		case Rti::RBF: filename = "rbf" + QString::number(parameters.nplanes); break;
		case Rti::BILINEAR: filename = "bln" + QString::number(parameters.nplanes); break;
		default: filename = "rti"; break;
		}
		if(parameters.colorspace == Rti::MYCC || parameters.colorspace == Rti::YCC)
			filename = "y" + filename + "." + QString::number(parameters.nchroma);
		if(parameters.format == RtiParameters::IIP)
			filename += ".tif";
	}
	input.cdUp();
	filename = input.filePath(filename);
	setPath(filename);
}

