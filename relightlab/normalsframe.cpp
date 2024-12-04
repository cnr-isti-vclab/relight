#include "normalsframe.h"
#include "normalstask.h"
#include "relightapp.h"
#include "processqueue.h"
#include "helpbutton.h"
#include "../src/project.h"
#include "normalsplan.h"

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


	content->addWidget(source_row = new NormalsSourceRow(parameters, this));
	content->addWidget(flatten_row = new NormalsFlattenRow(parameters, this));
	content->addWidget(surface_row = new NormalsSurfaceRow(parameters, this));


	{
		QHBoxLayout *save_row = new QHBoxLayout;

		{
			QLabel *label = new QLabel("");
			label->setFixedWidth(200);
			save_row->addWidget(label, 0, Qt::AlignLeft);
		}
		save_row->addStretch(1);

		{
			QFrame *buttons_frame = new QFrame;
			buttons_frame->setMinimumWidth(860);

			{
				QHBoxLayout *buttons_layout = new QHBoxLayout(buttons_frame);

				buttons_layout->addStretch(1);
				QPushButton *save = new QPushButton("Export", this);
				save->setIcon(QIcon::fromTheme("save"));
				save->setProperty("class", "large");
				save->setMinimumWidth(200);
				connect(save, &QPushButton::clicked, [this]() { this->save(); });

				buttons_layout->addWidget(save);
			}

			save_row->addWidget(buttons_frame);
		}
		save_row->addStretch(1);


		content->addLayout(save_row);

	}


	content->addStretch();

	return;
	content->addWidget(jpg = new QRadioButton("JPEG: normalmap"));
	content->addWidget(png = new QRadioButton("PNG: normalmap"));
	{
		QButtonGroup *group = new QButtonGroup(this);
		group->addButton(jpg);
		group->addButton(png);
		jpg->setChecked(true);
	}

	content->addWidget(new QLabel("<h2>Flatten normals</h2>"));
	content->addWidget(radial = new QCheckBox("Radial"));
	content->addWidget(fourier = new QCheckBox("Fourier"));
	{
		QHBoxLayout *fourier_layout = new QHBoxLayout;

		fourier_layout->addWidget(new HelpLabel("Fourier image percent: ", "normals/flatten"));
		fourier_layout->addWidget(fourier_radius = new QSpinBox);
		fourier_radius->setRange(0, 100);

		content->addLayout(fourier_layout);
	}

	content->addSpacing(30);
	content->addWidget(new QLabel("<h2>Export 3D surface</h2>"));
	content->addSpacing(30);
	{
		QHBoxLayout *discontinuity_layout = new QHBoxLayout;

		discontinuity_layout->addWidget(new HelpLabel("Disconitnuity parameter:", "normals/disconituity"));
		discontinuity_layout->addWidget(discontinuity = new QDoubleSpinBox);
		discontinuity->setRange(0.0, 4.0);
		discontinuity->setValue(2.0);
		content->addLayout(discontinuity_layout);
	}

	content->addWidget(tif = new QCheckBox("TIF: depthmap"));
	content->addWidget(ply = new QCheckBox("PLY: mesh"));


	QPushButton *save = new QPushButton("Export");
	content->addWidget(save);

	connect(save, SIGNAL(clicked()), this, SLOT(save()));

	content->addStretch();
}

void NormalsFrame::save() {
	if(parameters.compute && qRelightApp->project().dome.directions.size() == 0) {
		QMessageBox::warning(this, "Missing light directions.", "You need light directions for this dataset to build a normalmap.\n"
																"You can either load a dome or .lp file or mark a reflective sphere in the 'Lights' tab.");
		return;
	}

	NormalsTask *task = new NormalsTask();
	task->parameters = parameters;
	task->initFromProject(qRelightApp->project());

	ProcessQueue &queue = ProcessQueue::instance();
	queue.addTask(task);

	emit processStarted();
}
