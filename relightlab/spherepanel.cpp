#include "spherepanel.h"
#include "spherepicking.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>

SphereDialog::SphereDialog(QWidget *parent): QDialog(parent) {
	setModal(true);
	sphere_picking = new SpherePicking;
	sphere_picking->init();
	sphere_picking->imageMode();
	QVBoxLayout *content = new QVBoxLayout(this);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |     QDialogButtonBox::Cancel);

	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	content->addWidget(sphere_picking);
	content->addWidget(buttonBox);
	showMaximized();
}

void SphereDialog::accept() {
	QDialog::accept();
}

void SphereDialog::reject() {
	QDialog::reject();
}

SpherePanel::SpherePanel(QWidget *parent): QFrame(parent) {
	QVBoxLayout *content = new QVBoxLayout(this);

	content->addWidget(new QLabel("<h3>Mark a reflective sphere:</h3>"));
	content->addSpacing(30);

	QPushButton *new_sphere = new QPushButton("New sphere...");
	content->addWidget(new_sphere);
	connect(new_sphere, SIGNAL(clicked()), this, SLOT(newSphere()));
}

void SpherePanel::init() {
	//QVBoxLayout
}

void SpherePanel::newSphere() {
	if(!sphere_dialog)
		sphere_dialog = new SphereDialog(this);

	sphere_dialog->exec();
	//TODO check result
}

