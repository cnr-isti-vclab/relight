#include "queueitem.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include "task.h"

#include <iostream>
using namespace std;

QueueItem::QueueItem(Task *_task, QListWidget *parent): QListWidgetItem(parent) {

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

}

void QueueItem::progress(QString text, int percent) {
	progressbar->setValue(percent);
}

void QueueItem::update() {

}

void QueueItem::setSelected(bool selected) {
	if(selected)
		widget->setStyleSheet("background-color:  #ffa02f; color: #ffffff;");
	else
		widget->setStyleSheet("background-color:#323232; color:#b1b1b1");
}


