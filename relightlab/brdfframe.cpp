#include "brdfframe.h"
#include "brdfplan.h"
#include "brdftask.h"
#include "relightapp.h"
#include "reflectionview.h"
#include "helpbutton.h"
#include "processqueue.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QMessageBox>


BrdfFrame::BrdfFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *content = new QVBoxLayout(this);

	content->addWidget(new QLabel("<h2>BRDF creation</h2>"));
	content->addSpacing(30);

	content->addWidget(median_row = new BrdfMedianRow(parameters, this));
	content->addWidget(export_row = new BrdfExportRow(parameters, this));
	content->addStretch(1);

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

				zoom_view = new ZoomOverview(qRelightApp->project().crop, 200);
				buttons_layout->addWidget(zoom_view);

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

void BrdfFrame::init() {
	export_row->suggestPath();
	zoom_view->init();
	zoom_view->setCrop(qRelightApp->project().crop);
}

void BrdfFrame::updateCrop(Crop crop) {
	zoom_view->setCrop(crop);
}


void BrdfFrame::save() {


	BrdfTask *task = new BrdfTask();
	try {

		task->setParameters(parameters);
		task->output = parameters.path;
		task->initFromProject(qRelightApp->project());

	} catch(QString error) {
		QMessageBox::critical(this, "Something went wrong", error);
		delete task;
		return;
	}

	ProcessQueue &queue = ProcessQueue::instance();
	queue.addTask(task);

	emit processStarted();
}
