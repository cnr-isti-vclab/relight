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
	content->addWidget(export_row = new NormalsExportRow(parameters, this));


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
}

void NormalsFrame::init() {
	export_row->suggestPath();
}

void NormalsFrame::save() {
	if(parameters.compute) {
		//make sure we have some light directions defined
		if(qRelightApp->project().dome.directions.size() == 0) {
			QMessageBox::warning(this, "Missing light directions.", "You need light directions for this dataset to build a normalmap.\n"
																"You can either load a dome or .lp file or mark a reflective sphere in the 'Lights' tab.");
			return;
		}
	} else {
		if(parameters.flatMethod == FlatMethod::FLAT_NONE && parameters.surface_integration == SurfaceIntegration::SURFACE_NONE) {
			QMessageBox::warning(this, "Nothing to do.", "Using an existing normalmap ma no flattening or integration method specified");
			return;
		}

		if(parameters.input_path == parameters.path) {
			QMessageBox::warning(this, "Input and output normalmap have the same name", "The input normal map would be overwritten. Change the output filename");
			return;
		}
	}
	NormalsTask *task = new NormalsTask();
	task->setParameters(parameters);
	task->output = parameters.path;
	task->initFromProject(qRelightApp->project());

	ProcessQueue &queue = ProcessQueue::instance();
	queue.addTask(task);

	emit processStarted();
}
