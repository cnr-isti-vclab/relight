#include "alignrow.h"

#include "markerdialog.h"
#include "relightapp.h"
#include "verifydialog.h"
#include "reflectionview.h"
#include "../src/project.h"
#include "../src/align.h"
#include "processqueue.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>

FindAlignment::FindAlignment(Align *_align, bool update) {
	align = _align;
	update_positions = update;
	visible = false;
}

void FindAlignment::run() {
	setStatus(RUNNING);

	Project &project = qRelightApp->project();
	for(size_t i = 0; i < project.images.size(); i++) {

		Image &image = project.images[i];
		if(image.skip) continue;

		QImage img(image.filename);
		if(img.isNull()) {
			setStatus(FAILED);
			progressed(QString("Failed loading image: %1").arg(image.filename), 100);
			return;
		}
		align->readThumb(img, i);

		int progress = std::min(99, (int)(100*(i+1) / project.images.size()));
		if(!progressed(QString("Collecting patches"), progress))
			return;
	}
	progressed(QString("Done"), 100);
	setStatus(DONE);
}


AlignRow::AlignRow(Align *_align, QWidget *parent): QWidget(parent) {
	align = _align;
	QHBoxLayout *columns = new QHBoxLayout(this);
	columns->setSpacing(20);

	columns->addWidget(thumb = new QLabel());
	position = new AlignOverview(align->rect, rowHeight);
	position->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	columns->addWidget(position);


	/*reflections = new ReflectionView(sphere, rowHeight);
	reflections->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	columns->addWidget(reflections); */

	QVBoxLayout *status_layout = new QVBoxLayout;
	columns->addLayout(status_layout, 2);
	status_layout->addStretch();
	status = new QLabel("Loading patches...");
	status_layout->addWidget(status);
	progress = new QProgressBar;
	progress->setValue(0);
	status_layout->addWidget(progress);
	status_layout->addStretch();

	QGridLayout *edit_layout = new QGridLayout;
	columns->addLayout(edit_layout, 2);


	edit_layout->setRowStretch(0,1);

	region= new QLabel;
	updateRegion();
	edit_layout->addWidget(region, 1, 0, 1, 3);

	edit_button = new QPushButton(QIcon::fromTheme("edit"), "Edit...");
	edit_layout->addWidget(edit_button, 2, 0);
	verify_button = new QPushButton(QIcon::fromTheme("check"), "Verify...");
	verify_button->setEnabled(false);
	edit_layout->addWidget(verify_button, 2, 1);
	QPushButton *remove_button = new QPushButton(QIcon::fromTheme("trash-2"), "Delete");
	edit_layout->addWidget(remove_button, 2, 2);

	edit_layout->setRowStretch(3,1);

	connect(edit_button, &QPushButton::clicked, [this]() { emit edit(this); });
	connect(remove_button, &QPushButton::clicked, [this]() { emit removeme(this); });
	connect(verify_button, SIGNAL(clicked()), this, SLOT(verify()));
}

AlignRow::~AlignRow() {
	if(find_alignment) {
		stopFinding();
	}
	delete find_alignment;
}
/*
void AlignRow::edit() {
	MarkerDialog *marker_dialog = new MarkerDialog(MarkerDialog::ALIGN, this);
	marker_dialog->setAlign(align);
	int answer = marker_dialog->exec();
	if(answer == QDialog::Accepted) {
		position->rect = align->rect;
		position->update();
		updateRegion();
		findAlignment();
	}
}*/

void AlignRow::verify() {
	VerifyDialog *verify_dialog = new VerifyDialog(align->thumbs, align->offsets, VerifyDialog::ALIGN, this);
	verify_dialog->exec();
}


void AlignRow::updateRegion() {
	QRectF r = align->rect;
	region->setText(QString("Sample region: %1x%2+%3+%4").arg(r.width()).arg(r.height()).arg(r.left()).arg(r.top()));
}

void AlignRow::setRect(QRectF rect) {
	position->rect = align->rect;
	position->update();
	updateRegion();
	findAlignment();
}

void AlignRow::updateStatus(QString msg, int percent) {
	status->setText(msg);
	progress->setValue(percent);
	//reflections->update();
	if(percent == 100) {
		emit updated();
		verify_button->setEnabled(true);
	}
}

//AlignRow owns the task and is resnponsible of deleting it.

void AlignRow::findAlignment(bool update) {
	verify_button->setEnabled(false);
	if(!find_alignment) {
		find_alignment = new FindAlignment(align, update);
		connect(find_alignment, &FindAlignment::progress, this, &AlignRow::updateStatus); //, Qt::QueuedConnection);
	}
	find_alignment->stop();

	ProcessQueue &queue = ProcessQueue::instance();
	queue.removeTask(find_alignment);
	queue.addTask(find_alignment);
	queue.start();
}

void AlignRow::stopFinding() {
	if(find_alignment) {
		if(find_alignment->isRunning()) {
			find_alignment->stop();
			find_alignment->wait();
		}
		ProcessQueue &queue = ProcessQueue::instance();
		queue.removeTask(find_alignment);
	}
}
