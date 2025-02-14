#include "sphereframe.h"
#include "sphererow.h"
#include "markerdialog.h"
#include "spherepicking.h"
#include "relightapp.h"
#include "../src/sphere.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>


#include <assert.h>

SphereFrame::SphereFrame(QWidget *parent): QGroupBox("Reflective spheres", parent) {
	stack = new QStackedWidget;

	{
		QFrame *sphere_rows = new QFrame;
		QVBoxLayout *content = new QVBoxLayout(sphere_rows);
		content->addSpacing(10);

		{
			QPushButton *sphere = new QPushButton(QIcon::fromTheme("folder"), "New reflective sphere...");
			sphere->setProperty("class", "large");
			sphere->setMinimumWidth(200);
			sphere->setMaximumWidth(300);
			connect(sphere, SIGNAL(clicked()), this, SLOT(newSphere()));
			content->addWidget(sphere, 0, Qt::AlignTop);
		}
		{
			QFrame *spheres_frame = new QFrame;
			content->addWidget(spheres_frame);
			spheres = new QVBoxLayout(spheres_frame);
		}
		content->addStretch();

		stack->addWidget(sphere_rows);
	}
	{
		marker_dialog = new MarkerDialog(MarkerDialog::SPHERE, this);
		marker_dialog->setWindowFlags(Qt::Widget);
		connect(marker_dialog, SIGNAL(accepted()), this, SLOT(okMarker()));
		connect(marker_dialog, SIGNAL(rejected()), this, SLOT(cancelMarker()));
		stack->addWidget(marker_dialog);
	}

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(stack);
}

void SphereFrame::clear() {
	//setVisible(false);
	while(spheres->count() > 0) {
		QLayoutItem *item = spheres->takeAt(0);
		SphereRow *row =  dynamic_cast<SphereRow *>(item->widget());
		row->stopDetecting();
		delete row;
	}
}

void SphereFrame::init() {
	for(Sphere *sphere: qRelightApp->project().spheres) {
		sphere->fit();
		SphereRow * row = addSphere(sphere);
		row->detectHighlights(false);
	}
}



void SphereFrame::okMarker() {

	SphereRow *row = findRow(provisional_sphere);

	//TODO here the provisional_sphere should be updated
	if(!row) { //new align
		qRelightApp->project().spheres.push_back(provisional_sphere);
		row = addSphere(provisional_sphere);
	}
	row->detectHighlights();

	provisional_sphere = nullptr;
	stack->setCurrentIndex(0);

}

void SphereFrame::cancelMarker() {
	SphereRow *row = findRow(provisional_sphere);

	if(!row) //this was a new align cancelled
		delete provisional_sphere;

	provisional_sphere = nullptr;
	stack->setCurrentIndex(0);
}


/* on user button press */
void SphereFrame::newSphere() {
	stack->setCurrentIndex(1);
	provisional_sphere = new Sphere(qRelightApp->project().images.size());
	marker_dialog->setSphere(provisional_sphere);
}


void SphereFrame::editSphere(SphereRow *row) {
	stack->setCurrentIndex(1); //needs to be called before setAlign, for correct resize.
	provisional_sphere = row->sphere;
	marker_dialog->setSphere(provisional_sphere);
}

SphereRow *SphereFrame::addSphere(Sphere *sphere) {
	SphereRow *row = new SphereRow(sphere);
	spheres->addWidget(row);

	connect(row, SIGNAL(edit(SphereRow *)), this, SLOT(editSphere(SphereRow *)));
	connect(row, SIGNAL(removeme(SphereRow *)), this, SLOT(removeSphere(SphereRow *)));
	connect(row, SIGNAL(updated()), this, SIGNAL(updated()));
	return row;
}

void SphereFrame::removeSphere(SphereRow *row) {
	row->stopDetecting();

	layout()->removeWidget(row);

	Sphere *sphere = row->sphere;
	auto &spheres = qRelightApp->project().spheres;


	auto it = std::find(spheres.begin(), spheres.end(), sphere);

	assert(it != spheres.end());

	delete sphere;
	delete row;

	spheres.erase(it);
}

SphereRow *SphereFrame::findRow(Sphere *sphere) {
	for(int i = 0; i < spheres->count(); i++) {
		SphereRow *r = static_cast<SphereRow *>(spheres->itemAt(i)->widget());
		if(r->sphere == sphere)
			return r;
	}
	return nullptr;
}

