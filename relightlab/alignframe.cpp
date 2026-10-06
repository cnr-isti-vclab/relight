#include "alignframe.h"
#include "alignpicking.h"
#include "imageview.h"
#include "flowlayout.h"
#include "relightapp.h"
#include "markerdialog.h"
#include "alignrow.h"
#include "../src/align.h"
#include "../src/autoalign.h"
#include "../src/project.h"
#include "processqueue.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
//#include <QGraphicsRectItem>
#include <QPushButton>
#include <QProgressBar>
#include <QImageReader>
#include <QMessageBox>
#include <QStackedWidget>

#include <iostream>
using namespace std;

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

			auto_button = new QPushButton("Align automatically");
			auto_button->setProperty("class", "large");
			auto_button->setMinimumWidth(200);
			auto_button->setMaximumWidth(300);
			auto_button->setToolTip("Experimental: find the translation of each image.");
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

AutoAlignTask::AutoAlignTask() {
	visible = false;
	owned = true;
	label = "Aligning images.";
}

void AutoAlignTask::run() {
	setStatus(RUNNING);

	Project &project = qRelightApp->project();
	std::vector<int> used;
	for(size_t i = 0; i < project.images.size(); i++)
		if(!project.images[i].skip)
			used.push_back(int(i));

	auto load = [&](int i, QSize size) {
		QImageReader reader(project.images[used[i]].filename);
		reader.setAutoTransform(false);
		reader.setScaledSize(size);
		QImage img = reader.read();
		if(img.isNull())
			img = project.readImage(used[i]).scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
		return img;
	};
	std::function<bool(QString, int)> callback = [this](QString stage, int percent) { return progressed(stage, percent); };
	try {
		AutoAlign align(project.imgsize);
		std::vector<QPointF> found = align.run(int(used.size()), load, &callback);
		offsets.assign(project.images.size(), QPointF(0, 0));
		for(size_t k = 0; k < used.size(); k++)
			offsets[used[k]] = found[k];
		region = align.region;
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

void AlignFrame::clear() {
	if(auto_align && auto_align->isRunning()) {
		auto_align->stop();
		auto_align->wait();
	}
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
	stack->setCurrentIndex(0);
}

void AlignFrame::newAlign() {
	stack->setCurrentIndex(1);
	provisional_align = new Align(qRelightApp->project().images.size());
	marker_dialog->setAlign(provisional_align);
}


void AlignFrame::autoAlign() {
	if(!auto_align) {
		auto_align = new AutoAlignTask;
		connect(auto_align, &AutoAlignTask::progress, this, &AlignFrame::autoAlignProgress);
		connect(auto_align, &QThread::finished, this, &AlignFrame::autoAlignFinished);
	}
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
		QMessageBox::critical(this, "Could not align the images!", auto_align->error);
		return;
	}
	if(auto_align->status != Task::DONE)
		return;

	//the alignment is shown on a part of the area used, to check it with "Verify...".
	Project &project = qRelightApp->project();
	Align *align = new Align(project.images.size());
	QPoint center = auto_align->region.center();
	int side = std::min(400, auto_align->region.width());
	align->rect = QRect(center.x() - side/2, center.y() - side/2, side, side);
	align->offsets = auto_align->offsets;
	project.aligns.push_back(align);
	AlignRow *row = addAlign(align);
	project.cleanAlignCache();
	row->findAlignment();
	projectUpdate();
}

void AlignFrame::editAlign(AlignRow *row) {
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
