#include "rticard.h"
#include "helpbutton.h"
#include "rtiexportdialog.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>

RtiCard::RtiCard(Rti::Type _type, Rti::ColorSpace _colorspace, int _nplanes, int _nchroma, QWidget *parent):
	QFrame(parent), type(_type), colorspace(_colorspace), nplanes(_nplanes), nchroma(_nchroma) {

	
	
	QStringList titles;
	titles <<
		"PTM" <<
		"HSH" <<
		"RBF" <<
		"BNL" <<
		"Neural";

	QStringList tooltips;
	tooltips <<
		"Polynomial Texture Maps" <<
		"HemiSpherical Harmonics" <<
		"Radial Basis Functions" <<
		"Bilinear sampling" <<
		"Convolutional neural network";

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(title_label = new HelpLabel("", "rti/" + titles[(int)type].toLower()));
	updateTitle();

	//QImage thumb = qRelightApp->thumbnails()[0];
	QLabel *img = new QLabel();
	img->setMinimumSize(300, 300);
	img->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	layout->addWidget(img);

	if(type == Rti::HSH) {
		QComboBox *combo = new QComboBox;
		combo->addItems(QStringList() << "4 harmonics" << "9 harmonics");
		connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), [&] (int h) {
			nplanes = h == 0? 12 : 27;
			updateTitle();
		});
		layout->addWidget(combo);
	}	
	if(type == Rti::RBF || type == Rti::BILINEAR) {
		QComboBox *combo = new QComboBox;
		combo->addItems(QStringList() << "12 coefficients" << "15 coefficients" << "18 coefficients" << "21 coefficients" << "24 coefficients" << "27 coefficients");
		combo->setCurrentIndex(nplanes/3 - 4);

		layout->addWidget(combo);

		QComboBox *chroma = new QComboBox;
		chroma->addItems(QStringList() << "0 chroma dedicated" << "1 chroma dedicated" << "2 chroma dedicated");
		chroma->setCurrentIndex(nchroma);
		layout->addWidget(chroma);
	}

	QHBoxLayout *hbox = new QHBoxLayout;
	layout->addLayout(hbox);

	//hbox->addWidget(new QPushButton("Preview"), 1);
	QPushButton *create  = new QPushButton("Create");
	hbox->addWidget(create, 1);

	if(type == Rti::PTM) {
		QPushButton *legacy = new QPushButton("Export .ptm for RtiViewer");
		hbox->addWidget(legacy, 1);
	}
	if(type == Rti::HSH) {
		QPushButton *legacy = new QPushButton("Export .rti for RtiViewer");
		hbox->addWidget(legacy, 1);
	}




	connect(create, SIGNAL(clicked()), this, SLOT(rtiExport()));
		
	setFrameStyle(QFrame::StyledPanel);
	setAutoFillBackground(true);
	setBackgroundRole(QPalette::AlternateBase);
}

void RtiCard::updateTitle() {
	QStringList titles;
	titles <<
		"PTM" <<
		"HSH" <<
		"RBF" <<
		"BNL" <<
		"Neural";

	QString title = titles[(int)type];
	if(colorspace == Rti::LRGB) {
		title = "L" + title;
	}

	title += QString(" %1").arg(nplanes);
	if(nchroma > 0) {
		title += QString(".%1").arg(nchroma);
	}
	title_label->label->setText(title);
}

void RtiCard::rtiExport() {
	RtiExportDialog *dialog = new RtiExportDialog(this);
	dialog->exec();
}

void RtiCard::mousePressEvent(QMouseEvent *event) {
	if(checkable) {
		setChecked(!checked);
		emit toggled(checked);
	}
	QFrame::mousePressEvent(event);

}

void  RtiCard::setChecked(bool _checked) {
	checked = _checked;
	setFrameStyle(checked ? QFrame::Panel : QFrame::StyledPanel);
	setBackgroundRole(checked ? QPalette::Base : QPalette::AlternateBase);
}
