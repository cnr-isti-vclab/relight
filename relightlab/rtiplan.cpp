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
#include <QFileDialog>
#include <QMessageBox>

#include <iostream>
using namespace std;
RtiPlanRow::RtiPlanRow(RtiParameters &param, QFrame *parent): QFrame(parent), parameters(param) {
	QHBoxLayout *layout = new QHBoxLayout(this);

	label = new HelpLabel("", "");
	label->setFixedWidth(200);
	layout->addWidget(label, 0, Qt::AlignLeft);
	//layout->setSpacing(20);

	layout->addStretch(1);

	buttonsFrame = new QFrame;
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
	default:
		break; //TODO check MYCC!
	}
}


RtiPlanesRow::RtiPlanesRow(RtiParameters &parameters, QFrame *parent): RtiPlanRow(parameters, parent) {

	label->label->setText("Planes:");
	label->help->setId("rti/planes");


	buttons->addWidget(new QLabel("Total number of images:"));
	nplanesbox = new QComboBox;
	nplanesbox->setFixedWidth(100);
	for(int i = 0; i < 7; i++) {
		nplanesbox->addItem(QString::number(nimages[i]), QVariant(nimages[i]));
	}
	connect(nplanesbox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int n) {
		setNPlanes(nimages[n]*3, true);
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

void RtiPlanesRow::forceNPlanes(int nplanes) {
	QStandardItemModel *model =	  qobject_cast<QStandardItemModel *>(nplanesbox->model());
	Q_ASSERT(model != nullptr);
	for(int i = 0; i < 7; i++) {
		QStandardItem *item = model->item(i);
		bool disabled = nplanes >= 0 && nplanes != nimages[i]*3;
		item->setFlags(disabled ? item->flags() & ~Qt::ItemIsEnabled
								: item->flags() | Qt::ItemIsEnabled);
	}
}

void RtiPlanesRow::setNPlanes(int nplanes, bool emitting) {
	nchromabox->setEnabled(parameters.colorspace == Rti::YCC);

	parameters.nplanes = nplanes;
	if(emitting) {
		emit nplanesChanged();
		return;
	}
	for(int i = 0; i < nplanesbox->count(); i++) {
		if(nplanes/3 == nplanesbox->itemData(i).toInt())
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

RtiWebLayoutRow::RtiWebLayoutRow(RtiParameters &parameters, QFrame *parent):
	RtiPlanRow(parameters, parent) {
	//reparent buttons to make space for export openlime viewer checkbox
	QVBoxLayout *content = new QVBoxLayout;
	delete buttons;

	buttonsFrame->setLayout(content);
	buttons = new QHBoxLayout;
	content->addLayout(buttons);

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

	QCheckBox *openlime = new QCheckBox("Add openlime viewer code.");
	openlime->setChecked(parameters.openlime);
	connect(openlime, &QCheckBox::stateChanged, [this](int state) { this->parameters.openlime = state > 0; });
	content->addWidget(openlime);
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

