#include "normalsframe.h"
#include "normalstask.h"
#include "relightapp.h"
#include "processqueue.h"
#include "helpbutton.h"
#include "../src/project.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QFileDialog>
#include <QMessageBox>

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

	content->addWidget(new QLabel("<h2>Flatten normals</h2>"));
	content->addWidget(radial = new QCheckBox("Radial"));
	content->addWidget(fourier = new QCheckBox("Fourier"));
	QHBoxLayout *fourier_layout = new QHBoxLayout;
	content->addLayout(fourier_layout);

	fourier_layout->addWidget(new HelpLabel("Fourier image percent: ", "normals/flatten"));
	fourier_layout->addWidget(fourier_radius = new QSpinBox);
	fourier_radius->setRange(0, 100);


	content->addSpacing(30);
	content->addWidget(new QLabel("<h2>Export 3D surface</h2>"));

	content->addSpacing(30);
	QHBoxLayout *discontinuity_layout = new QHBoxLayout;
	content->addLayout(discontinuity_layout);

	discontinuity_layout->addWidget(new HelpLabel("Disconitnuity parameter:", "normals/disconituity"));
	discontinuity_layout->addWidget(discontinuity = new QDoubleSpinBox);
	discontinuity->setRange(0.0, 4.0);
	discontinuity->setValue(2.0);

	content->addWidget(tif = new QCheckBox("TIF: depthmap"));
	content->addWidget(ply = new QCheckBox("PLY: mesh"));



	QPushButton *save = new QPushButton("Export");
	content->addWidget(save);

	connect(save, SIGNAL(clicked()), this, SLOT(save()));

	content->addStretch();
}

void NormalsFrame::save() {
	if(qRelightApp->project().dome.directions.size() == 0) {
		QMessageBox::warning(this, "Missing light directions.", "You need light directions for this dataset to build a normalmap.\n"
			"You can either load a dome or .lp file or mark a reflective sphere in the 'Lights' tab.");
		return;
	}
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

	if(radial->isChecked())
		task->flatMethod = RADIAL;

	if(fourier->isChecked()) {
		task->flatMethod = FOURIER;
		task->m_FlatRadius = fourier_radius->value()/100.0f;
	}

	task->initFromProject(project);

	ProcessQueue &queue = ProcessQueue::instance();
	queue.addTask(task);

	emit processStarted();
}
