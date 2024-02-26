#include "rtiframe.h"
#include "helpbutton.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QLabel>

RtiFrame::RtiFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *content = new QVBoxLayout(this);

	QHBoxLayout *parameters = new QHBoxLayout;
	content->addLayout(parameters);

	QGroupBox *model = new QGroupBox("Model");
	parameters->addWidget(model);
	QGridLayout *model_layout = new QGridLayout(model);
	QRadioButton *ptm = new QRadioButton("Polynomial Texture Maps (PTM)");
	model_layout->addWidget(ptm, 0, 0);
	model_layout->addWidget(new HelpButton("rti/ptm"), 0, 1);

	QRadioButton *hsh = new QRadioButton("HemiSpherical Harmonics (HSH)");
	model_layout->addWidget(hsh, 1, 0);
	model_layout->addWidget(new HelpButton("rti/hsh"), 1, 1);

	QRadioButton *rbf = new QRadioButton("Radial Basis Functions (RBF)");
	model_layout->addWidget(rbf, 2, 0);
	model_layout->addWidget(new HelpButton("rti/rbf"), 2, 1);

	QRadioButton *bln = new QRadioButton("Bilinear sampling (BNL)");
	model_layout->addWidget(bln, 3, 0);
	model_layout->addWidget(new HelpButton("rti/bln"), 3, 1);

	QRadioButton *neural = new QRadioButton("Neural networkr");
	model_layout->addWidget(neural, 4, 0);
	model_layout->addWidget(new HelpButton("rti/neural"), 4, 1);


	QGroupBox *colorspace= new QGroupBox("Colorspace");
	parameters->addWidget(colorspace);
	QGridLayout *colorspace_layout = new QGridLayout(colorspace);
	QRadioButton *rgb = new QRadioButton("RGB");
	colorspace_layout->addWidget(rgb, 0, 0);
	colorspace_layout->addWidget(new HelpButton("rti/rgb"), 0, 1);

	QRadioButton *lrgb = new QRadioButton("LRGB");
	colorspace_layout->addWidget(lrgb, 1, 0);
	colorspace_layout->addWidget(new HelpButton("rti/lrgb"), 1, 1);

	QRadioButton *mrgb = new QRadioButton("MRGB");
	colorspace_layout->addWidget(mrgb, 2, 0);
	colorspace_layout->addWidget(new HelpButton("rti/mrgb"), 2, 1);

	QRadioButton *ycc = new QRadioButton("YCC");
	colorspace_layout->addWidget(ycc, 3, 0);
	colorspace_layout->addWidget(new HelpButton("rti/ycc"), 3, 1);


	QGroupBox *planes = new QGroupBox("Planes");
	parameters->addWidget(planes);

	QGridLayout *planes_layout = new QGridLayout(planes);
	planes_layout->addWidget(new QLabel("Total number of planes:"), 0, 0);

	QSpinBox *total_planes = new QSpinBox;
	planes_layout->addWidget(total_planes, 0, 1);

	planes_layout->addWidget(new QLabel("Number of luminance planes:"), 1, 0);

	QSpinBox *luminance_planes = new QSpinBox;
	planes_layout->addWidget(luminance_planes, 1, 1);


	content->addStretch();
}

void RtiFrame::init() {

}
