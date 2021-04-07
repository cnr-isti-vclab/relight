#include "queueitem.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include "task.h"

#include <iostream>
using namespace std;

QueueItem::QueueItem(Task *_task, QListWidget *parent): QListWidgetItem(parent) {
	style[Task::ON_QUEUE] = "background-color:#323232; color:#b1b1b1";
	style[Task::RUNNING] = "background-color:#328232; color:#b1b1b1";
	style[Task::PAUSED] = "background-color:#323232; color:#b1b1b1";
	style[Task::STOPPED] = "background-color:#823232; color:#b1b1b1";
	style[Task::DONE] = "background-color:#323282; color:#b1b1b1";
	style[Task::FAILED] = "background-color:#823232; color:#b1b1b1";

	task = _task;
	connect(task, SIGNAL(progress(QString,int)), this, SLOT(progress(QString,int)));

	id = task->id;
	widget = new QWidget;
	widget->setObjectName("task");

	QGridLayout *grid = new QGridLayout();

	QLabel *label = new QLabel(task->label);
	grid->addWidget(label, 0, 0, 1, 2);

	QFont font = label->font();
	font.setPointSize(10);

	QLabel *input = new QLabel(task->input_folder);
	input->setFont(font);
	grid->addWidget(input, 1, 0, 1, 2);

	QLabel *output = new QLabel(task->output);
	output->setFont(font);
	grid->addWidget(output, 2, 0, 1, 2);

	status = new QLabel();
	status->setMinimumWidth(250);
//	status->hide();
	grid->addWidget(status, 3, 0, 1, 1);

	progressbar = new QProgressBar();
	progressbar->setValue(0);
//	progressbar->hide();
	grid->addWidget(progressbar, 3, 1, 1, 1);

	trash = new QPushButton();
	trash->setIcon(QIcon(":/icons/feather/trash-2.svg"));
	grid->addWidget(trash, 0, 2, 4, 1);

	widget->setLayout(grid);

	setSizeHint(widget->minimumSizeHint());
	parent->setItemWidget(this, widget);

	update();
}

void QueueItem::progress(QString text, int percent) {
	status->setText(text);
	progressbar->setValue(percent);
}

void QueueItem::update() {
	cout << "UPdating: " << task->status << endl;
	switch(task->status) {
	case Task::PAUSED:
		status->setText("Paused");
	case Task::ON_QUEUE:
	case Task::RUNNING:
		break;
	case Task::DONE:
		status->setText("Done");
		progressbar->setValue(100);
		break;
	case Task::STOPPED:
		status->setText("Stopped");
		break;
	case Task::FAILED:
		status->setText(task->error);
		progressbar->setValue(0);
	}
	widget->setStyleSheet(style[task->status]);
}

void QueueItem::setSelected(bool selected) {
	if(selected)
		widget->setStyleSheet("background-color:#ffa02f; color: #ffffff;");
	else
		widget->setStyleSheet(style[task->status]);
}


