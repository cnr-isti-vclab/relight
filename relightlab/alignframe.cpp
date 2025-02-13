#include "alignframe.h"
#include "alignpicking.h"
#include "imageview.h"
#include "flowlayout.h"
#include "relightapp.h"
#include "markerdialog.h"
#include "alignrow.h"
#include "../src/align.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsRectItem>
#include <QPushButton>
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
			QPushButton *new_align = new QPushButton("New alignment...");
			new_align->setProperty("class", "large");
			new_align->setMinimumWidth(200);
			new_align->setMaximumWidth(300);
			connect(new_align, SIGNAL(clicked()), this, SLOT(newAlign()));
			content->addWidget(new_align);
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

	provisional_align->rect = marker_dialog->getAlign();
	if(!row) { //new align
		qRelightApp->project().aligns.push_back(provisional_align);
		row = addAlign(provisional_align);
	} else {
		row->setRect(provisional_align->rect);
	}
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


/* on user button press */
void AlignFrame::newAlign() {
	stack->setCurrentIndex(1);
	provisional_align = new Align(qRelightApp->project().images.size());
	marker_dialog->setAlign(provisional_align);
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

	cout << aligns->children().size() << endl;

	connect(row, SIGNAL(edit(AlignRow *)), this, SLOT(editAlign(AlignRow *)));
	connect(row, SIGNAL(removeme(AlignRow *)), this, SLOT(removeAlign(AlignRow *)));
	connect(row, SIGNAL(updated()), this, SLOT(projectUpdate()));
	return row;
}

void AlignFrame::removeAlign(AlignRow *row) {
	row->stopFinding();

	layout()->removeWidget(row);

	Align *align = row->align;
	auto &aligns = qRelightApp->project().aligns;

	auto it = std::find(aligns.begin(), aligns.end(), align);

	assert(it != aligns.end());

	delete align;
	aligns.erase(it);
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
