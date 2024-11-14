#include "normalsframe.h"
#include "normalstask.h"
#include "relightapp.h"
#include "processqueue.h"
#include "../src/project.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QFileDialog>

NormalsFrame::NormalsFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *content = new QVBoxLayout(this);
	content->addWidget(new QLabel("<h2>Export normals</h2>"));
	content->addSpacing(30);

	content->addWidget(jpg = new QRadioButton("JPEG: normalmap"));
	content->addWidget(png = new QRadioButton("PNG: normalmap"));
	QButtonGroup *group = new QButtonGroup(this);
	group->addButton(jpg);
	group->addButton(png);
	jpg->setChecked(true);

	content->addSpacing(30);
	content->addWidget(new QLabel("<h2>Export integrated surface</h2>"));

	content->addSpacing(30);
	content->addWidget(tif = new QCheckBox("TIF: depthmap"));
	content->addWidget(ply = new QCheckBox("PLY: mesh"));

	QPushButton *save = new QPushButton("Export");
	content->addWidget(save);

	connect(save, SIGNAL(clicked()), this, SLOT(save()));

	content->addStretch();
}

void NormalsFrame::save() {
	QString filter = jpg->isChecked() ? "JPEG Images (*.jpg)" : "PNG Images (*.png)";
	Project &project = qRelightApp->project();
	QString output = QFileDialog::getSaveFileName(this, "Select a filename for the normal map.", project.dir.path(), filter);
	if(output.isNull())
		return;
	QString extension = jpg->isChecked() ? ".jpg" : ".png";
	if(!output.endsWith(extension))
		output += extension;

	NormalsTask *task = new NormalsTask(output);
	if(ply->isChecked())
		task->exportPly = true;
	if(tif->isChecked())
		task->exportTiff = true;

	task->initFromProject(project);

	ProcessQueue &queue = ProcessQueue::instance();
	queue.addTask(task);

	emit processStarted();
}
