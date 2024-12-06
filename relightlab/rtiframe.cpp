#include "rtiframe.h"
#include "rtiplan.h"
#include "helpbutton.h"
#include "relightapp.h"
#include "rtirecents.h"
#include "qlabelbutton.h"


#include "processqueue.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QButtonGroup>
#include <QMessageBox>
#include <QLabel>
#include <QScrollArea>
#include <QFileDialog>




RtiFrame::RtiFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *content = new QVBoxLayout(this);

	content->addWidget(new QLabel("<h2>Build relightable images</h2>"));
	content->addSpacing(20);

	content->addWidget(recents = new RtiRecents);

	basis_row      = new RtiBasisRow(parameters, this);
	colorspace_row = new RtiColorSpaceRow(parameters, this);
	planes_row     = new RtiPlanesRow(parameters, this);
	format_row     = new RtiFormatRow(parameters, this);
	quality_row    = new RtiQualityRow(parameters, this);
	layout_row     = new RtiWebLayoutRow(parameters, this);
	export_row     = new RtiExportRow(parameters, this);


	content->addWidget(basis_row);
	content->addWidget(colorspace_row);
	content->addWidget(planes_row);
	content->addWidget(format_row);
	content->addWidget(quality_row);
	content->addWidget(layout_row);
	content->addWidget(export_row);



	connect(basis_row,      &RtiBasisRow::basisChanged,           this, &RtiFrame::basisChanged);
	connect(colorspace_row, &RtiColorSpaceRow::colorspaceChanged, this, &RtiFrame::colorspaceChanged);
	connect(planes_row,     &RtiPlanesRow::nplanesChanged,        this, &RtiFrame::nplanesChanged);
	connect(format_row,     &RtiFormatRow::formatChanged,         this, &RtiFrame::formatChanged);
	connect(layout_row,     &RtiWebLayoutRow::layoutChanged,      this, &RtiFrame::layoutChanged);
	connect(quality_row,    &RtiQualityRow::qualityChanged,       this, &RtiFrame::qualityChanged);

	{
		QHBoxLayout *save_row = new QHBoxLayout;

		{
			QLabel *label = new QLabel("");
			label->setFixedWidth(200);
			save_row->addWidget(label, 0, Qt::AlignLeft);
		}
		save_row->addStretch(1);

		{
			QFrame *buttons_frame = new QFrame;
			buttons_frame->setMinimumWidth(860);

			{
				QHBoxLayout *buttons_layout = new QHBoxLayout(buttons_frame);

				buttons_layout->addStretch(1);
				QPushButton *save = new QPushButton("Export", this);
				save->setIcon(QIcon::fromTheme("save"));
				save->setProperty("class", "large");
				save->setMinimumWidth(200);
				connect(save, &QPushButton::clicked, [this]() { exportRti(); });

				buttons_layout->addWidget(save);
			}

			save_row->addWidget(buttons_frame);
		}
		save_row->addStretch(1);


		content->addLayout(save_row);

	}

	content->addStretch();
}

void RtiFrame::init() {
	//fill in last RTI used or a common one.
	export_row->suggestPath();
}

void RtiFrame::exportRti() {

	//check for lights
	if(qRelightApp->project().dome.directions.size() == 0) {
		QMessageBox::warning(this, "Missing light directions.", "You need light directions for this dataset to build an RTI.\n"
																"You can either load a dome or .lp file or mark a reflective sphere in the 'Lights' tab.");
		return;
	}

	if(parameters.path.isEmpty()) {
		QMessageBox::warning(this, "Destination path is missing.", "Fill in the output folder or the filename for the RTI.");
		return;
	}
	RtiTask *rti_task = new RtiTask(qRelightApp->project());
	rti_task->setParameters(parameters);
	rti_task->output = parameters.path;

	ProcessQueue &queue = ProcessQueue::instance();
	queue.addTask(rti_task);

	emit processStarted();
}
void RtiFrame::updateNPlanes() {
	// PLANES

	auto &nplanes = parameters.nplanes;
	auto &nchroma = parameters.nchroma;

	switch(parameters.basis) {
	case Rti::PTM:
		nplanes = parameters.colorspace == Rti::RGB? 18: 9;
		nchroma = 0;
		planes_row->forceNPlanes(nplanes);
		break;
	case Rti::HSH:
		nplanes = parameters.colorspace == Rti::RGB? 27 : 12;
		nchroma = 0;
		planes_row->forceNPlanes(nplanes);
		break;
	case Rti::RBF:
	case Rti::BILINEAR:
		if(parameters.colorspace != Rti::YCC) nchroma = 0;
		planes_row->forceNPlanes(-1);
	default:
		break;
	}

	planes_row->setNPlanes(nplanes);
	planes_row->setNChroma(nchroma);
}

void RtiFrame::basisChanged() {
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

	updateNPlanes();

	//FORMAT
	format_row->allowLegacy(!pca);

	if(pca && parameters.format == RtiParameters::RTI) {
		format_row->setFormat(RtiParameters::WEB); //emit and cascade update other rows.
	}
	export_row->suggestPath();
}

void RtiFrame::colorspaceChanged() {

	updateNPlanes();

	bool pca = parameters.basis == Rti::RBF || parameters.basis == Rti::BILINEAR;
	if(pca && parameters.format == RtiParameters::RTI) {
		format_row->setFormat(RtiParameters::WEB);
	}
	export_row->suggestPath();

}

void RtiFrame::nplanesChanged() {
	export_row->suggestPath();
}

void RtiFrame::formatChanged() {
	bool legacy = quality_row->parameters.format == RtiParameters::RTI;
	//only RTI allows for lossless.
	quality_row->allowLossless(legacy);
	layout_row->setEnabled(quality_row->parameters.format == RtiParameters::WEB);
	export_row->suggestPath();
}

void RtiFrame::qualityChanged() {
}

void RtiFrame::layoutChanged() {

}


