#include "rtiframe.h"
#include "helpbutton.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>

RtiFrame::RtiFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *content = new QVBoxLayout(this);

	QVBoxLayout *parameters = new QVBoxLayout;
	content->addLayout(parameters);

	QGroupBox *model = new QGroupBox("Model");
	parameters->addWidget(model);
	QVBoxLayout *model_layout = new QVBoxLayout(model);
	model_layout->addWidget(new HelpRadio("Polynomial Texture Maps (PTM)", "rti/ptm"));
	model_layout->addWidget(new HelpRadio("HemiSpherical Harmonics (HSH)", "rti/hsh"));
	model_layout->addWidget(new HelpRadio("Radial Basis Functions (RBF)", "rti/rbf"));
	model_layout->addWidget(new HelpRadio("Bilinear sampling (BNL)", "rti/bln"));
	model_layout->addWidget(new HelpRadio("Neural network", "rti/neural"));

	QGroupBox *colorspace= new QGroupBox("Colorspace");
	parameters->addWidget(colorspace);
	QVBoxLayout *colorspace_layout = new QVBoxLayout(colorspace);
	colorspace_layout->addWidget(new HelpRadio("RGB", "rti/rgb"));
	colorspace_layout->addWidget(new HelpRadio("LRGB", "rti/lrgb"));
	colorspace_layout->addWidget(new HelpRadio("MRGB", "rti/mrgb"));
	colorspace_layout->addWidget(new HelpRadio("YCC", "rti/ycc"));

	QGroupBox *planes = new QGroupBox("Planes");
	parameters->addWidget(planes);

	QGridLayout *planes_layout = new QGridLayout(planes);
	planes_layout->addWidget(new HelpLabel("Total number of planes:", "rti/planes"), 0, 0);

	QSpinBox *total_planes = new QSpinBox;
	planes_layout->addWidget(total_planes, 0, 1);

	planes_layout->addWidget(new HelpLabel("Number of luminance planes:", "rti/luminance"), 1, 0);

	QSpinBox *luminance_planes = new QSpinBox;
	planes_layout->addWidget(luminance_planes, 1, 1);


	QGroupBox *legacy = new QGroupBox("Export legacy RTI");	
	parameters->addWidget(legacy);

	QGridLayout *legacy_layout = new QGridLayout(legacy);
	
	legacy_layout->addWidget(new HelpRadio("Lossless (heavy!)", "rti/legacy"), 0, 0);

	legacy_layout->addWidget(new HelpRadio("JPEG", "rti/legacy"), 1, 0);
	legacy_layout->addWidget(new HelpLabel("Quality:", "rti/legacy"), 1, 1);
	QSpinBox *quality = new QSpinBox;
	legacy_layout->addWidget(quality, 1, 2);

	legacy_layout->addWidget(new QLabel("Filename:"), 2, 0);	
	legacy_layout->addWidget(new QLineEdit(), 2, 1);	
	legacy_layout->addWidget(new QPushButton("..."), 2, 2);	

	legacy_layout->addWidget(new QPushButton("Export"), 3, 2);	

	content->addStretch();
}

void RtiFrame::init() {

}
