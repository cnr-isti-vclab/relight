#include "rticard.h"
#include "helpbutton.h"

#include <QVBoxLayout>
#include <QLabel>

RtiCard::RtiCard(Rti::Type type, QWidget *parent): QFrame(parent) {

	Rti::Type basis[] = { Rti::PTM, Rti::HSH, Rti::RBF, Rti::BILINEAR, Rti::NEURAL };
	QStringList basis_titles;
	basis_titles <<
		"PTM\nPolynomial Texture Maps" <<
		"HSH\nHemiSpherical Harmonics" <<
		"RBF\nRadial Basis Functions" <<
		"BNL\nBilinear sampling" <<
		"Neural\nConvolutional neural network";

	QStringList basis_text;
	basis_text <<
		"Polynomial Texture Maps" <<
		"HemiSpherical Harmonics" <<
		"Radial Basis Functions" <<
		"Bilinear sampling" <<
		"Convolutional neural network";

	QString title = basis_titles[(int)type];

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(new QLabel(basis_titles[(int)type]));
	//QImage thumb = qRelightApp->thumbnails()[0];
	QLabel *img = new QLabel();
	img->setMinimumSize(200, 200);
	layout->addWidget(img);
	//img->setPixmap(QPixmap::fromImage(thumb.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
	layout->addWidget(new HelpLabel(basis_text[(int)type], "rti/" + title.toLower()));
	
	setFrameStyle(QFrame::StyledPanel);
	setAutoFillBackground(true);
	setBackgroundRole(QPalette::AlternateBase);
}

void RtiCard::mousePressEvent(QMouseEvent *event) {
	setChecked(!checked);
	QFrame::mousePressEvent(event);
	emit toggled(checked);
}

void  RtiCard::setChecked(bool _checked) {
	checked = _checked;
	setFrameStyle(checked ? QFrame::Panel : QFrame::StyledPanel);
	setBackgroundRole(checked ? QPalette::Base : QPalette::AlternateBase);
}
