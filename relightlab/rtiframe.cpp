#include "rtiframe.h"
#include "helpbutton.h"
#include "relightapp.h"
#include "rticard.h"
#include "flowlayout.h"
#include "rtiplan.h"
#include "rtirow.h"
#include "rtirecents.h"
#include "rtiframe.h"
#include "qlabelbutton.h"
#include "rtitask.h"

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

#include "rtiplan.h"


RtiFrame::RtiFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *content = new QVBoxLayout(this);

	content->addWidget(new QLabel("<h2>Build relightable images</h2>"));
	content->addSpacing(20);

	content->addWidget(recents = new RtiRecents);

	content->addWidget(rti_plan = new RtiPlan, 1);
	connect(rti_plan, SIGNAL(exportRti()), this, SLOT(exportRti()));
	return;

	QScrollArea *area = new QScrollArea;
	area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	area->setWidgetResizable(true);
	content->addWidget(area, 1);

	QWidget *widget = new QWidget;
	area->setWidget(widget);


	FlowLayout *sample_layout = new FlowLayout(area->widget());
	
	sample_layout->addWidget(new RtiCard(Rti::PTM, Rti::LRGB, 9, 0));
	sample_layout->addWidget(new RtiCard(Rti::PTM, Rti::RGB, 18, 0));
	sample_layout->addWidget(new RtiCard(Rti::HSH, Rti::RGB, 27, 0));


	sample_layout->addWidget(new RtiCard(Rti::RBF, Rti::MRGB, 18, 0));
	sample_layout->addWidget(new RtiCard(Rti::BILINEAR, Rti::MRGB, 18, 0));



	/*
	QGroupBox *model = new QGroupBox("Model");
	content->addWidget(model);
	QVBoxLayout *model_layout = new QVBoxLayout(model);
	model_layout->addWidget(new HelpRadio("Polynomial Texture Maps (PTM)", "rti/ptm"));
	model_layout->addWidget(new HelpRadio("HemiSpherical Harmonics (HSH)", "rti/hsh"));
	model_layout->addWidget(new HelpRadio("Radial Basis Functions (RBF)", "rti/rbf"));
	model_layout->addWidget(new HelpRadio("Bilinear sampling (BNL)", "rti/bln"));
	model_layout->addWidget(new HelpRadio("Neural network", "rti/neural"));

	QGroupBox *colorspace= new QGroupBox("Colorspace");
	content->addWidget(colorspace);
	QVBoxLayout *colorspace_layout = new QVBoxLayout(colorspace);
	colorspace_layout->addWidget(new HelpRadio("RGB", "rti/rgb"));
	colorspace_layout->addWidget(new HelpRadio("LRGB", "rti/lrgb"));
	colorspace_layout->addWidget(new HelpRadio("MRGB", "rti/mrgb"));
	colorspace_layout->addWidget(new HelpRadio("YCC", "rti/ycc"));

	QGroupBox *planes = new QGroupBox("Planes");
	content->addWidget(planes);

	QGridLayout *planes_layout = new QGridLayout(planes);
	planes_layout->addWidget(new HelpLabel("Total number of planes:", "rti/planes"), 0, 0);

	QSpinBox *total_planes = new QSpinBox;
	planes_layout->addWidget(total_planes, 0, 1);

	planes_layout->addWidget(new HelpLabel("Number of luminance planes:", "rti/luminance"), 1, 0);

	QSpinBox *luminance_planes = new QSpinBox;
	planes_layout->addWidget(luminance_planes, 1, 1);
*/
/*
	QGroupBox *legacy = new QGroupBox("Export .rti for RtiViewer");
	content->addWidget(legacy);

	QGridLayout *legacy_layout = new QGridLayout(legacy);
	
	legacy_layout->addWidget(new HelpRadio("Lossless (heavy!)", "rti/legacy"), 0, 0);

	legacy_layout->addWidget(new HelpRadio("JPEG", "rti/legacy"), 1, 0);
	legacy_layout->addWidget(new HelpLabel("Quality:", "rti/legacy"), 1, 1);
	QSpinBox *quality = new QSpinBox;
	legacy_layout->addWidget(quality, 1, 2);

	legacy_layout->addWidget(new QLabel("Filename:"), 2, 0);	
	legacy_layout->addWidget(new QLineEdit(), 2, 1);	
	legacy_layout->addWidget(new QPushButton("..."), 2, 2);	

	legacy_layout->addWidget(new QPushButton("Export"), 3, 2);	 */

	content->addStretch();
}

void RtiFrame::exportRti() {

	//check for lights
	if(qRelightApp->project().dome.directions.size() == 0) {
		QMessageBox::warning(this, "Missing light directions.", "You need light directions for this dataset to build an RTI.\n"
			"You can either load a dome or .lp file or mark a reflective sphere in the 'Lights' tab.");
		return;
	}
	RtiParameters &parameters = rti_plan->parameters;
	//get folder if not legacy.
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
		output = QFileDialog::getSaveFileName(this, "Select a file name", QString(), label);
		if(output.isNull()) return;

		if(!output.endsWith(extension))
		output += extension;

	} else {
		output = QFileDialog::getSaveFileName(this, "Select an output folder", QString());
		if(output.isNull()) return;
	}
	parameters.path = output;

	RtiTask *rti_task = new RtiTask(qRelightApp->project());
	rti_task->parameters = parameters;

	ProcessQueue &queue = ProcessQueue::instance();
	queue.addTask(rti_task);

	emit processStarted();
}


