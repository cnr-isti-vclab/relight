#include "sphererow.h"
#include "relightapp.h"
#include "spheredialog.h"
#include "reflectionview.h"
#include "../src/project.h"
#include "../src/sphere.h"
#include "../relight/processqueue.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>

DetectHighlights::DetectHighlights(Sphere *_sphere) {
	sphere = _sphere;
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
		sphere->findHighlight(img, i);

		progressed(QString("Detecting highlights"), 100*(i+1) / project.images.size());
	}
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

	QVBoxLayout *status_layout = new QVBoxLayout;
	columns->addLayout(status_layout, 2);
	status_layout->addStretch();
	status = new QLabel("Locating highlights...");
	status_layout->addWidget(status);
	progress = new QProgressBar;
	progress->setValue(0);
	status_layout->addWidget(progress);
	status_layout->addStretch();

	QPushButton *edit = new QPushButton(QIcon::fromTheme("edit"), "Edit...");
	columns->addWidget(edit, 1);
	QPushButton *verify = new QPushButton(QIcon::fromTheme("check"), "Verify...");
	columns->addWidget(verify, 1);
	QPushButton *remove = new QPushButton(QIcon::fromTheme("trash-2"), "Delete");
	columns->addWidget(remove, 1);

	connect(edit, SIGNAL(clicked()), this, SLOT(edit()));

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

void SphereRow::updateStatus(QString msg, int percent) {
	status->setText(msg);
	progress->setValue(percent);
	reflections->update();
}

void SphereRow::detectHighlights() {
	if(sphere->center.isNull()) {
		status->setText("Needs at least 3 points.");
		return;
	}
	detect_highlights = new DetectHighlights(sphere);
	connect(detect_highlights, &DetectHighlights::progress, this, &SphereRow::updateStatus); //, Qt::QueuedConnection);

	ProcessQueue &queue = ProcessQueue::instance();
	queue.addTask(detect_highlights);
	queue.start();
}
