#include "markerdialog.h"
#include "spherepicking.h"
#include "alignpicking.h"
#include "imageview.h"

#include <QVBoxLayout>
#include <QDialogButtonBox>

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
	align_picking->setAlign(align);
}

void MarkerDialog::setSphere(Sphere *sphere) {
	sphere_picking->setSphere(sphere);
}


void MarkerDialog::accept() {
	QDialog::accept();
}

void MarkerDialog::reject() {
	QDialog::reject();
}
