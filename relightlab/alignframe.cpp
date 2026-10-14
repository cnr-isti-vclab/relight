#include "alignframe.h"
#include "alignpicking.h"
#include "imageview.h"
#include "flowlayout.h"
#include "relightapp.h"
#include "markerdialog.h"
#include "alignrow.h"
#include "../src/align/align.h"
#include "../src/align/autoalign.h"
#include "../src/project.h"
#include "processqueue.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
//#include <QGraphicsRectItem>
#include <QPushButton>
#include <QProgressBar>
#include <QMessageBox>
#include <QStackedWidget>

#include <algorithm>

AlignFrame::AlignFrame(QWidget *parent): QFrame(parent) {

	stack = new QStackedWidget;

	{
		QFrame *align_rows = new QFrame;
		QVBoxLayout *content = new QVBoxLayout(align_rows);
		content->addSpacing(10);
		{
			QHBoxLayout *buttons = new QHBoxLayout;
			QPushButton *new_align = new QPushButton("New alignment...");
			new_align->setProperty("class", "large");
			new_align->setMinimumWidth(200);
			new_align->setMaximumWidth(300);
			connect(new_align, SIGNAL(clicked()), this, SLOT(newAlign()));
			buttons->addWidget(new_align);

			auto_button = new QPushButton("Find alignment candidates...");
			auto_button->setProperty("class", "large");
			auto_button->setMinimumWidth(200);
			auto_button->setMaximumWidth(300);
			auto_button->setToolTip("Select a region, then find phase-correlation candidates between image pairs.");
			connect(auto_button, SIGNAL(clicked()), this, SLOT(autoAlign()));
			buttons->addWidget(auto_button);

			auto_progress = new QProgressBar;
			auto_progress->setMaximumWidth(300);
			auto_progress->hide();
			buttons->addWidget(auto_progress);
			buttons->addStretch();
			content->addLayout(buttons);
		}
		{
			QFrame *aligns_frame = new QFrame;
			content->addWidget(aligns_frame, 0);
			aligns = new QVBoxLayout(aligns_frame);
		}
		content->addStretch(1);

		stack->addWidget(align_rows);
	}

	{
		marker_dialog = new MarkerDialog(MarkerDialog::ALIGN, this);
		marker_dialog->setWindowFlags(Qt::Widget);
		connect(marker_dialog, SIGNAL(accepted()), this, SLOT(okMarker()));
		connect(marker_dialog, SIGNAL(rejected()), this, SLOT(cancelMarker()));
		stack->addWidget(marker_dialog);
	}

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(stack);
}

void AlignFrame::clear() {
	if(auto_align && auto_align->isRunning()) {
		auto_align->stop();
		auto_align->wait();
	}
	if(provisional_align && !findRow(provisional_align))
		delete provisional_align;
	provisional_align = nullptr;
	selecting_assisted_region = false;
	while(aligns->count() > 0) {
		QLayoutItem *item = aligns->takeAt(0);
		AlignRow *row =  dynamic_cast<AlignRow *>(item->widget());
		row->stopFinding();
		delete row;
	}
}

void AlignFrame::init() {
	for(Align *align: qRelightApp->project().aligns) {
		AlignRow *row = addAlign(align);
		row->findAlignment(false);
	}
}

void AlignFrame::okMarker() {
	if(selecting_assisted_region) {
		QRect selected_region = marker_dialog->getAlign().toRect();
		delete provisional_align;
		provisional_align = nullptr;
		selecting_assisted_region = false;
		stack->setCurrentIndex(0);
		startAutoAlign(selected_region);
		return;
	}

	AlignRow *row = findRow(provisional_align);

	provisional_align->rect = marker_dialog->getAlign().toRect();
	if(!row) { //new align
		qRelightApp->project().aligns.push_back(provisional_align);
		row = addAlign(provisional_align);
	} else {
		row->setRect(provisional_align->rect);
	}
	qRelightApp->project().cleanAlignCache();
	row->findAlignment();

	provisional_align = nullptr;
	stack->setCurrentIndex(0);

}

void AlignFrame::cancelMarker() {
	AlignRow *row = findRow(provisional_align);

	if(!row) //this was a new align cancelled
		delete provisional_align;

	provisional_align = nullptr;
	selecting_assisted_region = false;
	stack->setCurrentIndex(0);
}

void AlignFrame::newAlign() {
	selecting_assisted_region = false;
	stack->setCurrentIndex(1);
	provisional_align = new Align(qRelightApp->project().images.size());
	marker_dialog->setAlign(provisional_align);
}


void AlignFrame::autoAlign() {
	Project &project = qRelightApp->project();
	int side = std::min(400, std::min(project.imgsize.width(), project.imgsize.height()));
	provisional_align = new Align(project.images.size());
	provisional_align->rect = QRect(
		(project.imgsize.width() - side)/2,
		(project.imgsize.height() - side)/2,
		side,
		side
	);
	selecting_assisted_region = true;
	marker_dialog->setAlign(provisional_align);
	stack->setCurrentIndex(1);
}

void AlignFrame::startAutoAlign(QRect region) {
	if(!auto_align) {
		auto_align = new AlignTask(qRelightApp->project());
		connect(auto_align, &AlignTask::progress, this, &AlignFrame::autoAlignProgress);
		connect(auto_align, &QThread::finished, this, &AlignFrame::autoAlignFinished);
	}
	auto_align->selected_region = region;
	auto_align->correspondences.clear();
	auto_button->setEnabled(false);
	auto_progress->setValue(0);
	auto_progress->show();

	ProcessQueue &queue = ProcessQueue::instance();
	queue.removeTask(auto_align);
	queue.addTask(auto_align);
	queue.start();
}

void AlignFrame::autoAlignProgress(QString /*msg*/, int percent) {
	auto_progress->setValue(percent);
}

void AlignFrame::autoAlignFinished() {
	auto_button->setEnabled(true);
	auto_progress->hide();
	if(auto_align->status == Task::FAILED) {
		QMessageBox::critical(this, "Could not find alignment candidates!", auto_align->error);
		return;
	}
	if(auto_align->status != Task::DONE)
		return;

	Project &project = qRelightApp->project();
	Align *alignment = new Align(int(project.images.size()));
	alignment->rect = auto_align->selected_region;
	for(size_t i = 0; i < auto_align->translations.size(); i++) {
		auto t = auto_align->translations[i];
		alignment->offsets[size_t(auto_align->image_indices[i])] = QPointF(t.x(), t.y());
	}
	project.aligns.push_back(alignment);
	AlignRow *row = addAlign(alignment);
	project.cleanAlignCache();
	row->findAlignment();
}

void AlignFrame::editAlign(AlignRow *row) {
	selecting_assisted_region = false;
	stack->setCurrentIndex(1); //needs to be called before setAlign, for correct resize.
	provisional_align = row->align;
	marker_dialog->setAlign(provisional_align);
}

void AlignFrame::projectUpdate() {
	auto &project = qRelightApp->project();
	project.computeOffsets();
}

AlignRow *AlignFrame::addAlign(Align *align) {
	AlignRow *row = new AlignRow(align);
	aligns->addWidget(row);

	connect(row, SIGNAL(editme(AlignRow *)),   this, SLOT(editAlign(AlignRow *)));
	connect(row, SIGNAL(removeme(AlignRow *)), this, SLOT(removeAlign(AlignRow *)));
	connect(row, SIGNAL(updated()),            this, SLOT(projectUpdate()));
	return row;
}

void AlignFrame::removeAlign(AlignRow *row) {

	layout()->removeWidget(row);
	delete row;

	projectUpdate();
}

AlignRow *AlignFrame::findRow(Align *align) {
	for(int i = 0; i < aligns->count(); i++) {
		AlignRow *r = static_cast<AlignRow *>(aligns->itemAt(i)->widget());
		if(r->align == align)
			return r;
	}
	return nullptr;
}
