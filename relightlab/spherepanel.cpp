#include "spherepanel.h"
#include "sphererow.h"
#include "spheredialog.h"
#include "spherepicking.h"
#include "relightapp.h"
#include "../src/sphere.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDebug>

#include <assert.h>

SpherePanel::SpherePanel(QWidget *parent): QGroupBox("Reflective spheres", parent) {
	//setFrameShape(QFrame::Box);
	QVBoxLayout *content = new QVBoxLayout(this);

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
		spheres = new QVBoxLayout;
		spheres_frame->setLayout(spheres);
		content->addWidget(spheres_frame);
	}
}

void SpherePanel::clear() {
	//setVisible(false);
	while(spheres->count() > 0) {
		QLayoutItem *item = spheres->takeAt(0);
		SphereRow *row =  dynamic_cast<SphereRow *>(item->widget());
		row->stopDetecting();
		delete row;
	}
}

void SpherePanel::init() {
	auto &project_spheres = qRelightApp->project().spheres;
	//setVisible(project_spheres.size() > 0);
	for(Sphere *sphere: project_spheres) {
		sphere->fit();
		SphereRow * row = addSphere(sphere);
		row->detectHighlights(false);
	}
}

/* on user button press */
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
	setVisible(true);
	SphereRow *row = new SphereRow(sphere);
	spheres->addWidget(row);

	connect(row, SIGNAL(removeme(SphereRow *)), this, SLOT(removeSphere(SphereRow *)));
	connect(row, SIGNAL(updated()), this, SIGNAL(updated()));
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
	delete row;

	spheres.erase(it);
	setVisible(spheres.size());

}

