#include "spherepanel.h"
#include "sphererow.h"
#include "spheredialog.h"
#include "spherepicking.h"
#include "relightapp.h"
#include "../src/sphere.h"
#include "lightgeometry.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDebug>

#include <assert.h>

SpherePanel::SpherePanel(QWidget *parent): QFrame(parent) {
	QVBoxLayout *content = new QVBoxLayout(this);

	content->addSpacing(10);
	QPushButton *new_sphere = new QPushButton("New sphere...");
	new_sphere->setProperty("class", "large");
	content->addWidget(new_sphere);
	new_sphere->setMinimumWidth(200);
	new_sphere->setMaximumWidth(300);

	QFrame *spheres_frame = new QFrame;
	content->addWidget(spheres_frame);
	spheres = new QVBoxLayout(spheres_frame);

	//content->addStretch();
	connect(new_sphere, SIGNAL(clicked()), this, SLOT(newSphere()));
}

void SpherePanel::init() {
	for(Sphere *sphere: qRelightApp->project().spheres) {
		sphere->fit();
		addSphere(sphere);
	}
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
	connect(row, SIGNAL(removeme(SphereRow *)), this, SLOT(removeSphere(SphereRow *)));
	return row;
}

void SpherePanel::removeSphere(SphereRow *row) {
	layout()->removeWidget(row);

	row->stopDetecting();

	Sphere *sphere = row->sphere;
	auto &spheres = qRelightApp->project().spheres;

	auto it = std::find(spheres.begin(), spheres.end(), sphere);

	assert(it != spheres.end());

	delete sphere;
	spheres.erase(it);
	delete row;
}
