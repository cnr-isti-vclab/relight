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
#include <QDebug>

DetectHighlights::DetectHighlights(Sphere *_sphere, bool update) {
	sphere = _sphere;
	update_positions = update;
	visible = false;
	label = "Detecting sphere highlights.";
}

void DetectHighlights::run() {
	mutex.lock();
	status = RUNNING;
	mutex.unlock();

	Project &project = qRelightApp->project();
	for(size_t i = 0; i < project.images.size(); i++) {

		Image &image = project.images[i];
		if(image.skip) continue;

		QImage img(image.filename);
		sphere->findHighlight(img, i, update_positions);

		int progress = std::min(99, (int)(100*(i+1) / project.images.size()));
		progressed(QString("Detecting highlights"), progress);
	}
	progressed(QString("Done."), 100);
	mutex.lock();
	status = DONE;
	mutex.unlock();
}


SphereRow::SphereRow(Sphere *_sphere, QWidget *parent): QWidget(parent) {
	sphere = _sphere;
	QHBoxLayout *columns = new QHBoxLayout(this);
	columns->setSpacing(20);

	position = new PositionView(sphere, rowHeight);
	position->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	columns->addWidget(position);


	reflections = new ReflectionView(sphere, rowHeight);
	reflections->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	columns->addWidget(reflections);

	progress = new QProgressBar;
	progress->setValue(0);
	columns->addWidget(progress, 2);

	QPushButton *edit = new QPushButton(QIcon::fromTheme("edit"), "Edit...");
	columns->addWidget(edit, 1);
	QPushButton *verify = new QPushButton(QIcon::fromTheme("check"), "Verify...");
	columns->addWidget(verify, 1);
	QPushButton *remove = new QPushButton(QIcon::fromTheme("trash-2"), "Delete");
	columns->addWidget(remove, 1);

	connect(edit, SIGNAL(clicked()), this, SLOT(edit()));
	connect(remove, SIGNAL(clicked()), this, SLOT(remove()));
	connect(verify, SIGNAL(clicked()), this, SLOT(verify()));

}
void SphereRow::edit() {
	SphereDialog *sphere_dialog = new SphereDialog(this);
	sphere_dialog->setSphere(sphere);
	int answer = sphere_dialog->exec();
	if(answer == QDialog::Accepted) {
		position->update();
		reflections->init();
		detectHighlights();
	}
}

void SphereRow::verify() {
	std::vector<QPointF> &positions = sphere->lights;
	for(QPointF &pos: positions)
		pos -= sphere->inner.topLeft();

	VerifyDialog *verify_dialog = new VerifyDialog(sphere->thumbs, positions, this);
	verify_dialog->exec();

	for(QPointF &pos: positions)
		pos += sphere->inner.topLeft();
}

void SphereRow::remove() {
	emit removeme(this);
}

void SphereRow::updateStatus(QString msg, int percent) {
//	status->setText(msg);
	progress->setValue(percent);
	reflections->update();
	if(percent == 100) {
		emit updated();
	}
}

void SphereRow::detectHighlights(bool update) {
	if(sphere->center.isNull()) {
//		status->setText("Needs at least 3 points.");
		return;
	}
	if(!detect_highlights) {
		detect_highlights = new DetectHighlights(sphere, update);
		connect(detect_highlights, &DetectHighlights::progress, this, &SphereRow::updateStatus); //, Qt::QueuedConnection);
	}
	detect_highlights->stop();

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
		detect_highlights->deleteLater();
	}
}
