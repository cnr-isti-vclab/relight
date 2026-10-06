#include "sphereframe.h"
#include "sphererow.h"
#include "markerdialog.h"
#include "spherepicking.h"
#include "relightapp.h"
#include "processqueue.h"
#include "../src/sphere.h"
#include "../src/project.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QStackedWidget>
#include <QImageReader>
#include <QMessageBox>
#include <QLineF>


#include <assert.h>

SphereFrame::SphereFrame(QWidget *parent): QGroupBox("Reflective spheres", parent) {
	stack = new QStackedWidget;

	{
		QFrame *sphere_rows = new QFrame;
		QVBoxLayout *content = new QVBoxLayout(sphere_rows);
		content->addSpacing(10);

		{
			QHBoxLayout *buttons = new QHBoxLayout;
			QPushButton *sphere = new QPushButton(QIcon::fromTheme("folder"), "New reflective sphere...");
			sphere->setProperty("class", "large");
			sphere->setMinimumWidth(200);
			sphere->setMaximumWidth(300);
			connect(sphere, SIGNAL(clicked()), this, SLOT(newSphere()));
			buttons->addWidget(sphere);

			locate_button = new QPushButton(QIcon::fromTheme("zoom-in"), "Locate spheres");
			locate_button->setProperty("class", "large");
			locate_button->setMinimumWidth(200);
			locate_button->setMaximumWidth(300);
			locate_button->setToolTip("Find the reflective spheres automatically from the highlights.");
			connect(locate_button, SIGNAL(clicked()), this, SLOT(locateSpheres()));
			buttons->addWidget(locate_button);

			locate_progress = new QProgressBar;
			locate_progress->setMaximumWidth(300);
			locate_progress->hide();
			buttons->addWidget(locate_progress);
			buttons->addStretch();
			content->addLayout(buttons);
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

LocateSpheres::LocateSpheres() {
	visible = false;
	owned = true;
	label = "Locating reflective spheres.";
}

void LocateSpheres::run() {
	setStatus(RUNNING);
	circles.clear();

	Project &project = qRelightApp->project();
	std::vector<int> used;
	for(size_t i = 0; i < project.images.size(); i++)
		if(!project.images[i].skip)
			used.push_back(int(i));

	auto load = [&](int i, QSize size, QRect clip) {
		QImageReader reader(project.images[used[i]].filename);
		reader.setAutoTransform(false);
		if(size.isValid())
			reader.setScaledSize(size);
		if(clip.isValid())
			reader.setClipRect(clip);
		QImage img = reader.read();
		if(img.isNull()) {
			img = project.readImage(used[i]);
			if(clip.isValid())
				img = img.copy(clip);
		}
		return img;
	};
	std::function<bool(QString, int)> callback = [this](QString stage, int percent) { return progressed(stage, percent); };
	try {
		SphereLocator locator(project.imgsize);
		circles = locator.run(int(used.size()), load, &callback);
	} catch(QString e) {
		if(status != STOPPED) {
			error = e;
			setStatus(FAILED);
		}
		return;
	}
	progressed("Done.", 100);
	setStatus(DONE);
}

void SphereFrame::clear() {
	if(locate_spheres && locate_spheres->isRunning()) {
		locate_spheres->stop();
		locate_spheres->wait();
	}
	while(spheres->count() > 0) {
		QLayoutItem *item = spheres->takeAt(0);
		SphereRow *row =  dynamic_cast<SphereRow *>(item->widget());
		row->stopDetecting();
		delete row;
	}
}

void SphereFrame::init() {
	stack->setCurrentIndex(0);
	marker_dialog->clear();
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
	qRelightApp->project().cleanSphereCache();
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


void SphereFrame::locateSpheres() {
	if(!locate_spheres) {
		locate_spheres = new LocateSpheres;
		connect(locate_spheres, &LocateSpheres::progress, this, &SphereFrame::locateProgress);
		connect(locate_spheres, &QThread::finished, this, &SphereFrame::locateFinished);
	}
	locate_button->setEnabled(false);
	locate_progress->setValue(0);
	locate_progress->show();

	ProcessQueue &queue = ProcessQueue::instance();
	queue.removeTask(locate_spheres);
	queue.addTask(locate_spheres);
	queue.start();
}

void SphereFrame::locateProgress(QString /*msg*/, int percent) {
	locate_progress->setValue(percent);
}

void SphereFrame::locateFinished() {
	locate_button->setEnabled(true);
	locate_progress->hide();
	if(locate_spheres->status == Task::FAILED) {
		QMessageBox::critical(this, "Could not locate spheres!", locate_spheres->error);
		return;
	}
	if(locate_spheres->status != Task::DONE)
		return;

	Project &project = qRelightApp->project();
	std::vector<SphereRow *> rows;
	for(SphereLocator::Circle &circle: locate_spheres->circles) {
		bool known = false;
		for(Sphere *sphere: project.spheres)
			if(QLineF(sphere->center, circle.center).length() < sphere->radius + circle.radius)
				known = true;
		if(known)
			continue;

		Sphere *sphere = new Sphere(project.images.size());
		sphere->border = circle.points();
		sphere->fit();
		project.spheres.push_back(sphere);
		rows.push_back(addSphere(sphere));
	}
	if(rows.empty()) {
		QMessageBox::information(this, "Locate spheres", locate_spheres->circles.empty() ?
			"No reflective sphere found, use \"New reflective sphere...\" to mark it." :
			"No new reflective sphere found.");
		return;
	}
	project.cleanSphereCache();
	for(SphereRow *row: rows)
		row->detectHighlights();
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

	connect(row, SIGNAL(editme(SphereRow *)), this, SLOT(editSphere(SphereRow *)));
	connect(row, SIGNAL(removeme(SphereRow *)), this, SLOT(removeSphere(SphereRow *)));
	connect(row, SIGNAL(updated()), this, SIGNAL(updated()));
	return row;
}

void SphereFrame::removeSphere(SphereRow *row) {

	layout()->removeWidget(row);
	delete row;

	// Emit updated signal so lights can be recomputed
	emit updated();
}

SphereRow *SphereFrame::findRow(Sphere *sphere) {
	for(int i = 0; i < spheres->count(); i++) {
		SphereRow *r = static_cast<SphereRow *>(spheres->itemAt(i)->widget());
		if(r->sphere == sphere)
			return r;
	}
	return nullptr;
}

