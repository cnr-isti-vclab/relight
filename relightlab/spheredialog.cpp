#include "spheredialog.h"
#include "spherepicking.h"
#include "imageview.h"

#include <QVBoxLayout>
#include <QDialogButtonBox>

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
	QDialog::accept();
}

void SphereDialog::reject() {
	QDialog::reject();
}
