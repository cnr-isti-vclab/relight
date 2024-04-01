#include "alignrow.h"
#include "markerdialog.h"
#include "relightapp.h"
#include "verifydialog.h"
#include "reflectionview.h"
#include "../src/project.h"
#include "../src/sphere.h"
#include "../relight/processqueue.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QDebug>

FindAlignment::FindAlignment(Align *_align, bool update) {
	align = _align;
	update_positions = update;
}

void FindAlignment::run() {
	mutex.lock();
	status = RUNNING;
	mutex.unlock();
/*
	Project &project = qRelightApp->project();
	for(size_t i = 0; i < project.images.size(); i++) {

		Image &image = project.images[i];
		if(image.skip) continue;

		QImage img(image.filename);
		sphere->findHighlight(img, i, update_positions);

		int progress = std::min(99, (int)(100*(i+1) / project.images.size()));
		progressed(QString("Detecting highlights"), progress);
	} */
	progressed(QString("Done"), 100);
	mutex.lock();
	status = DONE;
	mutex.unlock();
}


AlignRow::AlignRow(Align *_align, QWidget *parent): QWidget(parent) {
	align = _align;
	QHBoxLayout *columns = new QHBoxLayout(this);
	columns->setSpacing(20);

	columns->addWidget(thumb = new QLabel());
/*	position = new PositionView(sphere, rowHeight);
	position->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	columns->addWidget(position);


	reflections = new ReflectionView(sphere, rowHeight);
	reflections->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	columns->addWidget(reflections); */

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
	connect(remove, SIGNAL(clicked()), this, SLOT(remove()));
	connect(verify, SIGNAL(clicked()), this, SLOT(verify()));

}
void AlignRow::edit() {
	MarkerDialog *marker_dialog = new MarkerDialog(MarkerDialog::ALIGN, this);
	marker_dialog->setAlign(align);
	int answer = marker_dialog->exec();
	if(answer == QDialog::Accepted) {
		//position->update();
		//reflections->init();
		//detectHighlights();
	}
}

void AlignRow::verify() {
	std::vector<QPointF> centers;
	std::vector<QImage> thumbs;
//	assert(0); //todo needs to initialize those vaules and update align.
	VerifyDialog *verify_dialog = new VerifyDialog(thumbs, centers, this);
	verify_dialog->exec();
}

void AlignRow::remove() {
	emit removeme(this);
}

void AlignRow::updateStatus(QString msg, int percent) {
	status->setText(msg);
	progress->setValue(percent);
	reflections->update();
	if(percent == 100) {
		emit updated();
	}
}

void AlignRow::findAlignment(bool update) {
/*	if(sphere->center.isNull()) {
		status->setText("Needs at least 3 points.");
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
	queue.start(); */
}

void AlignRow::stopFinding() {
	/*
	if(detect_highlights) {
		if(detect_highlights->isRunning()) {
			detect_highlights->stop();
			detect_highlights->wait();
		}
		detect_highlights->deleteLater();
	} */
}
