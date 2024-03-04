#include "rtiframe.h"
#include "helpbutton.h"
#include "relightapp.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QButtonGroup>
#include <QLabel>

class RtiCard: public QFrame {
public:
	QPushButton *button;

	RtiCard(QString title, QString text, QWidget *parent = nullptr) {
		QVBoxLayout *layout = new QVBoxLayout(this);
		layout->addWidget(button = new QPushButton(title));
		button->setCheckable(true);
		//QImage thumb = qRelightApp->thumbnails()[0];
		QLabel *img = new QLabel();
		img->setMinimumSize(200, 200);
		layout->addWidget(img);
		//img->setPixmap(QPixmap::fromImage(thumb.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
		layout->addWidget(new HelpLabel(text, title));
		setFrameStyle(QFrame::StyledPanel);
	}
	
};

RtiFrame::RtiFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *content = new QVBoxLayout(this);

	content->addWidget(new QLabel("<h2>Relightable images</h2>"));

	QHBoxLayout *basis_layout = new QHBoxLayout;
	content->addLayout(basis_layout);

	QButtonGroup *basis_group = new QButtonGroup;

	QStringList basis_labels;
	basis_labels <<
		"PTM\nPolynomial Texture Maps" <<
		"HSH\nHemiSpherical Harmonics" <<
		"RBF\nRadial Basis Functions" <<
		"BNL\nBilinear sampling" <<
		"Neural\nConvolutional neural network";
		
	basis_layout->addStretch(1);
	for(int i = 0; i < 5; i++) {
	/*	basis_layout->addWidget(basis[i] = new QPushButton(basis_labels[i]), 3);
		basis[i]->setCheckable(true);
		basis[i]->setProperty("class", "");

		basis[i]->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
		basis_group->addButton(basis[i], i); */
		RtiCard *card = new RtiCard(basis_labels[i].split("\n").first(), basis_labels[i].split("\n").last());
		basis_layout->addWidget(card, 2);
		basis_group->addButton(card->button, i);
	}
	basis_layout->addStretch(1);

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

	legacy_layout->addWidget(new QPushButton("Export"), 3, 2);	

	content->addStretch();
}

void RtiFrame::init() {

}
