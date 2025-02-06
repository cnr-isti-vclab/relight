#include "queueframe.h"

#include "processqueue.h"
#include "queueitem.h"
#include "relightapp.h"


#include <QMessageBox>
#include <QVBoxLayout>
#include <QListWidget>
#include <QAction>
#include <QToolBar>

#include <iostream>
using namespace std;



QueueFrame::QueueFrame(QWidget *parent): QFrame(parent) {

	QVBoxLayout *vbox = new QVBoxLayout(this);


	toolbar = new QToolBar;
	vbox->addWidget(toolbar);

	toolbar->addAction(actionStart = qRelightApp->addAction("queue_start", "Start", "play", ""));
	toolbar->addAction(actionPause = qRelightApp->addAction("queue_pause", "Pause", "pause", ""));
	toolbar->addAction(actionStop = qRelightApp->addAction("queue_stop", "Stop", "stop", ""));
	toolbar->addAction(actionToBottom = qRelightApp->addAction("queue_bottom", "Send to bottom", "chevrons-down", ""));
	toolbar->addAction(actionToTop = qRelightApp->addAction("queue_top", "Send to top", "chevrons-up", ""));
	toolbar->addAction(actionRemove = qRelightApp->addAction("queue_remove", "Remove", "trash-2", ""));
	toolbar->addAction(actionOpenFolder = qRelightApp->addAction("queue_open", "Open folder", "folder", ""));
	toolbar->addAction(actionInfo = qRelightApp->addAction("queue_info", "Info", "info", ""));



	list = new QListWidget;
	vbox->addWidget(list);

	list->setSelectionMode(QAbstractItemView::ExtendedSelection);
	connect(list->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
			this, SLOT(selectionChanged(QItemSelection, QItemSelection)));
	setToolsStatus();

	connect(actionStart,          SIGNAL(triggered(bool)), this, SLOT(start()));
	connect(actionPause,          SIGNAL(triggered(bool)), this, SLOT(pause()));
	connect(actionStop,           SIGNAL(triggered(bool)), this, SLOT(stop()));
	connect(actionToBottom, SIGNAL(triggered(bool)), this, SLOT(sendToTop()));
	connect(actionToTop,    SIGNAL(triggered(bool)), this, SLOT(sendToBottom()));
	connect(actionRemove,         SIGNAL(triggered(bool)), this, SLOT(remove()));

	ProcessQueue &queue = ProcessQueue::instance();
	connect(&queue, SIGNAL(update()), this, SLOT(update()));

	update();
}


void QueueFrame::setToolsStatus() {
	ProcessQueue &queue = ProcessQueue::instance();
	actionStart->setEnabled(queue.stopped == true);
	actionPause->setEnabled(queue.stopped == false);
	actionStop->setEnabled(queue.stopped == false || queue.task != nullptr);


	bool empty_selection = list->selectedItems().size() == 0;
	actionToBottom->setDisabled(empty_selection);
	actionToTop->setDisabled(empty_selection);
	actionRemove->setDisabled(empty_selection);
	actionInfo->setDisabled(empty_selection);
	//actionOpenFolder->setDisabled(empty_selection);
}


void QueueFrame::start() {
	ProcessQueue &queue = ProcessQueue::instance();
	queue.start();
	setToolsStatus();

}
void QueueFrame::pause() {
	ProcessQueue &queue = ProcessQueue::instance();
	queue.pause();
	setToolsStatus();

}
void QueueFrame::stop() {
	ProcessQueue &queue = ProcessQueue::instance();
	queue.stop();
	setToolsStatus();
}

void QueueFrame::sendToTop() {

}

void QueueFrame::sendToBottom() {

}

void QueueFrame::remove() {
	QStringList tasks;
	for(QModelIndex index: list->selectionModel()->selectedRows()) {
		QueueItem *item = (QueueItem *)list->item(index.row());
		QString label = item->task->label;
		if(item->task->status == Task::PAUSED)
			label += " (PAUSED)";
		if(item->task->status == Task::RUNNING)
			label += " (RUNNING)";
		tasks.push_back(item->task->label);
	}
	int answer = QMessageBox::question(this, "Removing tasks",
									   QString("Are you sure you want to remove this task:?\n") + tasks.join("\n"),
									   QMessageBox::Cancel, QMessageBox::Ok);
	if(answer == QMessageBox::No)
		return;
	ProcessQueue &queue = ProcessQueue::instance();

	QModelIndexList selection = list->selectionModel()->selection().indexes();
	std::sort(selection.begin(), selection.end(),
		  [](const QModelIndex &a, const QModelIndex &b) -> bool { return a.row() < b.row(); });

	while(!selection.isEmpty()) {
		QModelIndex i = selection.takeLast();
		QueueItem *item = (QueueItem *)list->item(i.row());
		if(item->task->status == Task::PAUSED ||item->task->status == Task::RUNNING) {
			queue.stop();
		}
		queue.removeTask(item->id);
		list->takeItem(i.row());
		delete item;
	}
}

void QueueFrame::removeTask(Task *task) {
	ProcessQueue &queue = ProcessQueue::instance();
	queue.removeTask(task->id);
	for(int i = 0; i < list->count(); i++) {
		QueueItem *item = dynamic_cast<QueueItem *>(list->item(i));
		if(item->id == task->id) {
			list->removeItemWidget(item);
			//should remove task from queue?
		}
	}
}

void QueueFrame::selectionChanged(const QItemSelection & selected, const QItemSelection & deselected) {
	for(QModelIndex index: deselected.indexes()) {
		int row = index.row();
		QueueItem *item = (QueueItem *)list->item(row);
		item->setSelected(false);
	}
	for(QModelIndex index: selected.indexes()) {
		int row = index.row();
		QueueItem *item = (QueueItem *)list->item(row);
		item->setSelected(true);
	}
	setToolsStatus();
}


void QueueFrame::update() {
	ProcessQueue &queue = ProcessQueue::instance();

	QSet<int> tasks;
	//check status of each widget
	for(int i = 0; i < list->count(); i++) {
		QueueItem *item = (QueueItem *)list->item(i);
		//check for deleted tasks
		if(!queue.contains(item->task)) {
			throw QString("Delete task!");
		}
		item->update();
		tasks.insert(item->id);
	}

	QMutexLocker lock(&queue.lock);
	//add all task not already present.
	for(Task *task: queue.queue) {
		if(!tasks.contains(task->id) && task->visible) {
			QueueItem *item = new QueueItem(task, list);
			list->addItem(item);
		}
	}
	/*if(queue.task && !tasks.contains(queue.task->id) && queue.task->visible) {
		QueueItem *item = new QueueItem(queue.task, list);
		list->addItem(item);
	} */
}
