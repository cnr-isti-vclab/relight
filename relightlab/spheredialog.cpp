#include "spheredialog.h"
#include "spherepicking.h"
#include "imageview.h"
#include "../src/sphere.h"

#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QMessageBox>

SphereDialog::SphereDialog(QWidget *parent): QDialog(parent) {
	setModal(true);
	sphere_picking = new SpherePicking;
	sphere_picking->showImage(0);
	QVBoxLayout *content = new QVBoxLayout(this);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |     QDialogButtonBox::Cancel);

	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	content->addWidget(sphere_picking);
	content->addWidget(buttonBox);
	showMaximized();
}

void SphereDialog::setSphere(Sphere *sphere) {
	sphere_picking->setSphere(sphere);
	sphere_picking->fit();
}


void SphereDialog::accept() {
	if(sphere_picking->sphere->border.size() < 3) {
		QMessageBox::information(this, "Not enought points", "At least 3 points are needed to fit a circle");
		return;
	}
	if(!sphere_picking->sphere->fitted) {
		QMessageBox::information(this, "Sphere not fit", "The sphere could not be fit, try to remove and add back the points");
		return;
	}
	//TODO check for a valid (inside the image) inner rectangle.
	QDialog::accept();
}

void SphereDialog::reject() {
	QDialog::reject();
}
