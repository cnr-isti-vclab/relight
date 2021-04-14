#include "queuewindow.h"
#include "ui_queuewindow.h"

#include "processqueue.h"
#include "queueitem.h"

#include <QMessageBox>

#include <iostream>
using namespace std;
QueueWindow::QueueWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::QueueWindow) {

	ProcessQueue &queue = ProcessQueue::instance();
	connect(&queue, SIGNAL(update()), this, SLOT(update()));

	ui->setupUi(this);

	ui->list->setSelectionMode(QAbstractItemView::ExtendedSelection);
	connect(ui->list->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
			this, SLOT(selectionChanged(QItemSelection, QItemSelection)));
	setToolsStatus();

	connect(ui->actionStart,          SIGNAL(triggered(bool)), this, SLOT(start()));
	connect(ui->actionPause,          SIGNAL(triggered(bool)), this, SLOT(pause()));
	connect(ui->actionStop,           SIGNAL(triggered(bool)), this, SLOT(stop()));
	connect(ui->actionSend_to_bottom, SIGNAL(triggered(bool)), this, SLOT(sendToTop()));
	connect(ui->actionSend_to_top,    SIGNAL(triggered(bool)), this, SLOT(sendToBottom()));
	connect(ui->actionRemove,         SIGNAL(triggered(bool)), this, SLOT(remove()));

	update();
}

QueueWindow::~QueueWindow() {
	delete ui;
}

void QueueWindow::setToolsStatus() {
	ProcessQueue &queue = ProcessQueue::instance();
	ui->actionStart->setEnabled(queue.stopped == true);
	ui->actionPause->setEnabled(queue.stopped == false);
	ui->actionStop->setEnabled(queue.stopped == false || queue.task != nullptr);


	bool empty_selection = ui->list->selectedItems().size() == 0;
	ui->actionSend_to_bottom->setDisabled(empty_selection);
	ui->actionSend_to_top->setDisabled(empty_selection);
	ui->actionRemove->setDisabled(empty_selection);
	ui->actionInfo->setDisabled(empty_selection);
	ui->actionOpen_folder->setDisabled(empty_selection);
}


void QueueWindow::start() {
	ProcessQueue &queue = ProcessQueue::instance();
	queue.start();
	setToolsStatus();

}
void QueueWindow::pause() {
	ProcessQueue &queue = ProcessQueue::instance();
	queue.pause();
	setToolsStatus();

}
void QueueWindow::stop() {
	ProcessQueue &queue = ProcessQueue::instance();
	queue.stop();
	setToolsStatus();
}

void QueueWindow::sendToTop() {

}

void QueueWindow::sendToBottom() {

}

void QueueWindow::remove() {
	QStringList tasks;
	for(QModelIndex index: ui->list->selectionModel()->selectedRows()) {
		QueueItem *item = (QueueItem *)ui->list->item(index.row());
		QString label = item->task->label;
		if(item->task->status == Task::PAUSED)
			label += " (PAUSED)";
		if(item->task->status == Task::RUNNING)
			label += " (RUNNING)";
		tasks.push_back(item->task->label);
	}
	int answer = QMessageBox::question(this, "Removing tasks",
									   QString("Are you sure you want to remove this tasks:?\n") + tasks.join("\n"),
									   QMessageBox::Cancel, QMessageBox::Ok);
	if(answer == QMessageBox::No)
		return;
	ProcessQueue &queue = ProcessQueue::instance();
	for(QModelIndex index: ui->list->selectionModel()->selectedRows()) {
		QueueItem *item = (QueueItem *)ui->list->item(index.row());
		if(item->task->status == Task::PAUSED ||item->task->status == Task::RUNNING) {
			queue.stop();
		}
		int id = item->id;
		delete ui->list->takeItem(index.row());
		queue.removeTask(item->id);
	}
}

void QueueWindow::selectionChanged(const QItemSelection & selected, const QItemSelection & deselected) {
	for(QModelIndex index: deselected.indexes()) {
		int row = index.row();
		QueueItem *item = (QueueItem *)ui->list->item(row);
		item->setSelected(false);
	}
	for(QModelIndex index: selected.indexes()) {
		int row = index.row();
		QueueItem *item = (QueueItem *)ui->list->item(row);
		item->setSelected(true);
	}
	setToolsStatus();
}


void QueueWindow::update() {
	QSet<int> tasks;
	//check status of each widget
	for(int i = 0; i < ui->list->count(); i++) {
		QueueItem *item = (QueueItem *)ui->list->item(i);
		item->update();
		tasks.insert(item->id);
	}
	ProcessQueue &queue = ProcessQueue::instance();
	//add all task not already presetn.
	for(Task *task: queue.queue) {
		if(!tasks.contains(task->id)) {
			QueueItem *item = new QueueItem(task, ui->list);
			ui->list->addItem(item);
		}
	}
	if(queue.task && !tasks.contains(queue.task->id)) {
		QueueItem *item = new QueueItem(queue.task, ui->list);
		ui->list->addItem(item);
	}
	for(Task *task: queue.past) {
		if(!tasks.contains(task->id)) {
			QueueItem *item = new QueueItem(task, ui->list);
			ui->list->addItem(item);

		}
	}

}
