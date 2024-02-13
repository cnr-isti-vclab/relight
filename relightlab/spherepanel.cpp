#include "spherepanel.h"
#include "sphererow.h"
#include "spheredialog.h"
#include "spherepicking.h"
#include "relightapp.h"
#include "../src/sphere.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

SpherePanel::SpherePanel(QWidget *parent): QFrame(parent) {
	QVBoxLayout *content = new QVBoxLayout(this);

	content->addWidget(new QLabel("<h3>Mark a reflective sphere:</h3>"));
	content->addSpacing(30);

	QPushButton *new_sphere = new QPushButton("New sphere...");
	content->addWidget(new_sphere);
	new_sphere->setMinimumWidth(200);
	new_sphere->setMaximumWidth(300);

	QFrame *spheres_frame = new QFrame;
	content->addWidget(spheres_frame);
	spheres = new QVBoxLayout(spheres_frame);

	content->addStretch(1);
	connect(new_sphere, SIGNAL(clicked()), this, SLOT(newSphere()));
}

void SpherePanel::init() {
	//QVBoxLayout
}

void SpherePanel::newSphere() {
	if(!sphere_dialog)
		sphere_dialog = new SphereDialog(this);

	//TODO ACTUALLY images might be skipped!
	Sphere *sphere = new Sphere(qRelightApp->project().images.size());
	sphere_dialog->setSphere(sphere);
	int answer = sphere_dialog->exec();
	if(answer == QDialog::Rejected) {
		delete sphere;
		return;
	}
	qRelightApp->project().spheres.push_back(sphere);
	SphereRow *row = addSphere(sphere);
	row->detectHighlights();
}

SphereRow *SpherePanel::addSphere(Sphere *sphere) {
	/*Row is: 1) thumbnail of the first image where the sphere is rendered.
			  2) detail thumb with all the reflections.
			  3) status
			  4) edit and delete button

	*/
	SphereRow *row = new SphereRow(sphere);
	spheres->addWidget(row);
	return row;
}

void SpherePanel::removeSphere(Sphere *sphere) {

}
