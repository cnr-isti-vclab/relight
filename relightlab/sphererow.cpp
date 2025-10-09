#include "sphererow.h"
#include "relightapp.h"
#include "spheredialog.h"
#include "verifydialog.h"
#include "reflectionview.h"
#include "../src/project.h"
#include "../src/sphere.h"
#include "processqueue.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QMessageBox>

DetectHighlights::DetectHighlights(Sphere *_sphere, bool update) {
	sphere = _sphere;
	update_positions = update;
	visible = false;
	owned = true;
	label = "Detecting sphere highlights.";
}


void DetectHighlights::run() {
	setStatus(RUNNING);

	QRect inner = sphere->inner;
	QString cache_filename = QString("resources/spherecache_%1x%2+%3+%4.jpg")
			.arg(inner.width())
			.arg(inner.height())
			.arg(inner.left())
			.arg(inner.top());

	if(!update_positions) {
		QImage img(cache_filename);
		if(!img.isNull()) {
			sphere->readCacheThumbs(img);
			progressed(QString("Done."), 100);
			setStatus(DONE);
			return;
		}
	}
	sphere->sphereImg.fill(0);

	Project &project = qRelightApp->project();
	for(size_t i = 0; i < project.images.size(); i++) {

		Image &image = project.images[i];

		QImage img;
		img.load(image.filename, "JPG");
		if(img.isNull()) {
			setStatus(FAILED);
			progressed(QString("Failed loading image: %1").arg(image.filename), 100);
			return;
		}
		sphere->findHighlight(img, i, image.skip, update_positions);

		int progress = std::min(99, (int)(100*(i+1) / project.images.size()));
		if(!progressed(QString("Detecting highlights"), progress))
			return;
	}
	//save final image:
	sphere->saveCacheThumbs(cache_filename);
	progressed(QString("Done."), 100);
	setStatus(DONE);
}

SphereRow::SphereRow(Sphere *_sphere, QWidget *parent): QWidget(parent) {
	sphere = _sphere;
	QHBoxLayout *columns = new QHBoxLayout(this);
	columns->setSpacing(20);

	position = new SphereOverview(sphere->center, sphere->radius, rowHeight);
	position->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	columns->addWidget(position);


	reflections = new ReflectionOverview(sphere, rowHeight);
	reflections->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	columns->addWidget(reflections);

	progress = new QProgressBar;
	progress->setValue(0);
	columns->addWidget(progress, 2);

	QPushButton *edit = new QPushButton(QIcon::fromTheme("edit"), "Edit sphere...");
	columns->addWidget(edit, 1);
	verify_button = new QPushButton(QIcon::fromTheme("check"), "Verify...");
	verify_button->setEnabled(false);
	columns->addWidget(verify_button, 1);
	QPushButton *remove = new QPushButton(QIcon::fromTheme("trash-2"), "Delete");
	columns->addWidget(remove, 1);

	connect(edit, &QPushButton::clicked, this, [this]() { emit editme(this); });
	connect(remove, SIGNAL(clicked()), this, SLOT(remove()));
	connect(verify_button, SIGNAL(clicked()), this, SLOT(verify()));
}

SphereRow::~SphereRow() {
	if(detect_highlights) {
		stopDetecting();
	}
	delete detect_highlights;
}

void SphereRow::verify() {
	std::vector<QPointF> &positions = sphere->lights;
	for(QPointF &pos: positions) {
		if(!pos.isNull())
			pos -= sphere->inner.center();
	}
	VerifyDialog *verify_dialog = new VerifyDialog(sphere->thumbs, positions, VerifyDialog::REFLECTION, this);
	verify_dialog->exec();

	for(QPointF &pos: positions) {
		if(!pos.isNull())
			pos += sphere->inner.center();
	}

	emit updated();
}

void SphereRow::remove() {
	stopDetecting();

	auto &spheres = qRelightApp->project().spheres;
	auto it = std::find(spheres.begin(), spheres.end(), sphere);
	assert(it != spheres.end());
	delete sphere;
	spheres.erase(it);

	qRelightApp->project().cleanSphereCache();

	emit removeme(this);
}

void SphereRow::updateStatus(QString msg, int percent) {
	if(detect_highlights->status == Task::FAILED) {
		QMessageBox::critical(this, "Could not detect highlights!", msg);
		progress->setValue(0);
		return;
	}
	progress->setValue(percent);
	reflections->update();
	if(percent == 100) {
		emit updated();
		verify_button->setEnabled(true);
	}
}

void SphereRow::detectHighlights(bool update) {

	if(sphere->center.isNull()) {
		return;
	}
	//look for cached data.
	verify_button->setEnabled(false);
	if(!detect_highlights) {
		detect_highlights = new DetectHighlights(sphere, update);
		connect(detect_highlights, &DetectHighlights::progress, this, &SphereRow::updateStatus); //, Qt::QueuedConnection);
	}
	detect_highlights->stop();
	detect_highlights->update_positions = update;

	ProcessQueue &queue = ProcessQueue::instance();
	queue.removeTask(detect_highlights);
	queue.addTask(detect_highlights);
	queue.start();
}


void SphereRow::stopDetecting() {
	if(detect_highlights) {
		if(detect_highlights->isRunning()) {
			detect_highlights->stop();
			detect_highlights->wait();
		}
		verify_button->setEnabled(true);
		ProcessQueue &queue = ProcessQueue::instance();
		queue.removeTask(detect_highlights);
	}
}
