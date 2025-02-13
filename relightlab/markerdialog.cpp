#include "markerdialog.h"
#include "spherepicking.h"
#include "alignpicking.h"
#include "imageview.h"

#include <QVBoxLayout>
#include <QDialogButtonBox>
#include "../src/align.h"

MarkerDialog::MarkerDialog(Marker marker, QWidget *parent): QDialog(parent) {

	QVBoxLayout *content = new QVBoxLayout(this);

	setModal(true);
	switch(marker) {
		case SPHERE:
			sphere_picking = new SpherePicking();
			content->addWidget(sphere_picking);

			break;
		case ALIGN:
			align_picking = new AlignPicking();
			content->addWidget(align_picking);
			break;
	}

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	content->addWidget(buttonBox);
	showMaximized();
}


void MarkerDialog::setAlign(Align *align) {
	align_picking->setAlign(align->rect);
}

QRectF MarkerDialog::getAlign() {
	return align_picking->getRect();

}
void MarkerDialog::setSphere(Sphere *sphere) {
	sphere_picking->setSphere(sphere);
}

void  MarkerDialog::clear() {
	if(align_picking) {
		align_picking->clear();
	}
	if(sphere_picking)
		sphere_picking->clear();

}

void MarkerDialog::accept() {
	QDialog::accept();
}

void MarkerDialog::reject() {
	QDialog::reject();
}
