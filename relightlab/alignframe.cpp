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

#include <iostream>
using namespace std;

AlignFrame::AlignFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *content = new QVBoxLayout(this);

	content->addSpacing(10);
	QPushButton *new_align = new QPushButton("New align...");
	new_align->setProperty("class", "large");
	content->addWidget(new_align);
	new_align->setMinimumWidth(200);
	new_align->setMaximumWidth(300);

	QFrame *aligns_frame = new QFrame;
	content->addWidget(aligns_frame, 0);
	aligns = new QVBoxLayout(aligns_frame);

	content->addStretch(1);
	//content->addStretch();
	connect(new_align, SIGNAL(clicked()), this, SLOT(newAlign()));
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

/* on user button press */
void AlignFrame::newAlign() {
	if(!marker_dialog)
		marker_dialog = new MarkerDialog(MarkerDialog::ALIGN, this);

	Align *align = new Align(qRelightApp->project().images.size());
	marker_dialog->setAlign(align);
	int answer = marker_dialog->exec();
	if(answer == QDialog::Rejected) {
		delete align;
		return;
	}
	qRelightApp->project().aligns.push_back(align);
	AlignRow *row = addAlign(align);
	row->findAlignment();
}

void AlignFrame::projectUpdate() {
	auto &project = qRelightApp->project();
	project.computeOffsets();

	auto offsets = qRelightApp->project().offsets;
	for(auto &o: offsets) {
		o -= project.offsets[0];
		cout << o.x() << " " << o.y() << endl;
	}

}

AlignRow *AlignFrame::addAlign(Align *align) {
	AlignRow *row = new AlignRow(align);
	aligns->addWidget(row);

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
