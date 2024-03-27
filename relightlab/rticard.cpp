#include "rticard.h"
#include "helpbutton.h"

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

	QString title = titles[(int)type];
	if(colorspace == Rti::LRGB) {
		title = "L" + title;
	}
	
	title += QString(" %1").arg(nplanes);
	if(nchroma > 0) {
		title += QString(".%1").arg(nchroma);
	}

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(new HelpLabel(title, "rti/" + titles[(int)type].toLower()));

	//QImage thumb = qRelightApp->thumbnails()[0];
	QLabel *img = new QLabel();
	img->setMinimumSize(300, 300);
	img->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	layout->addWidget(img);

	QHBoxLayout *hbox = new QHBoxLayout;
	layout->addLayout(hbox);

	hbox->addWidget(new QPushButton("Preview"), 1);
	hbox->addWidget(new QPushButton("Create..."), 1);

	hbox->addWidget(new QPushButton(QIcon::fromTheme("trash-2"), ""));
	//img->setPixmap(QPixmap::fromImage(thumb.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
		
	setFrameStyle(QFrame::StyledPanel);
	setAutoFillBackground(true);
	setBackgroundRole(QPalette::AlternateBase);
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
