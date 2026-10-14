#include "alignrow.h"

#include "relightapp.h"
#include "verifydialog.h"
#include "../src/project.h"
#include "../src/align/align.h"
#include "reflectionview.h"
#include "processqueue.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QMessageBox>
#include <cassert>
#include <algorithm>

AlignRow::AlignRow(Align *_align, QWidget *parent): QWidget(parent) {
	align = _align;
	QHBoxLayout *columns = new QHBoxLayout(this);
	columns->setSpacing(20);

	columns->addWidget(thumb = new QLabel());
	position = new AlignOverview(align->rect, rowHeight);
	position->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	columns->addWidget(position);


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
	edit_layout->addWidget(region, 1, 0, 1, 2);

	verify_button = new QPushButton(QIcon::fromTheme("check"), "Verify...");
	verify_button->setEnabled(false);
	edit_layout->addWidget(verify_button, 2, 0);
	QPushButton *remove_button = new QPushButton(QIcon::fromTheme("trash-2"), "Delete");
	edit_layout->addWidget(remove_button, 2, 1);

	edit_layout->setRowStretch(3,1);

	connect(remove_button, SIGNAL(clicked()), this, SLOT(remove()));
	connect(verify_button, SIGNAL(clicked()), this, SLOT(verify()));
}

AlignRow::~AlignRow() {
	if(find_alignment) {
		stopFinding();
	}
	delete find_alignment;
}

void AlignRow::verify() {
	VerifyDialog *verify_dialog = new VerifyDialog(align->thumbs, align->offsets, VerifyDialog::ALIGN, this);
	verify_dialog->exec();

	emit updated();
}

void AlignRow::remove() {
	stopFinding();
	auto &aligns = qRelightApp->project().aligns;
	auto it = std::find(aligns.begin(), aligns.end(), align);
	assert(it != aligns.end());

	delete align;
	aligns.erase(it);

	qRelightApp->project().cleanAlignCache();

	emit removeme(this);
}

void AlignRow::updateRegion() {
	QRectF r = align->rect;
	region->setText(QString("Sample region: %1x%2+%3+%4").arg(r.width()).arg(r.height()).arg(r.left()).arg(r.top()));
}

void AlignRow::updateStatus(QString msg, int percent) {
	if(find_alignment->status == Task::FAILED) {
		QMessageBox::critical(this, "Could not find alignment!", msg);
		progress->setValue(0);
		return;
	}

	status->setText(msg);
	progress->setValue(percent);
	if(percent == 100) {
		emit updated();
		verify_button->setEnabled(true);
	}
}

//AlignRow owns the task and is resnponsible of deleting it.

void AlignRow::findAlignment(bool update) {
	verify_button->setEnabled(false);
	status->setText("Loading patches...");
	progress->setValue(0);
	if(!find_alignment) {
		find_alignment = new AlignTask(qRelightApp->project(), align);
		connect(find_alignment, &AlignTask::progress, this, &AlignRow::updateStatus);
	} else {
		stopFinding();
	}
	verify_button->setEnabled(false);
	find_alignment->update_positions = update;

	ProcessQueue &queue = ProcessQueue::instance();
	queue.addTask(find_alignment);
	queue.start();
}

void AlignRow::stopFinding() {
	if(find_alignment) {
		if(find_alignment->isRunning()) {
			find_alignment->stop();
			find_alignment->wait();
		}
		verify_button->setEnabled(true);
		ProcessQueue &queue = ProcessQueue::instance();
		queue.removeTask(find_alignment);
	}
}
