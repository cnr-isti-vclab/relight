#include "queueframe.h"

#include "processqueue.h"
#include "queueitem.h"
#include "relightapp.h"
#include "historytask.h"


#include <QVBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QMutexLocker>
#include <QToolBar>
#include <QAction>
#include <QJsonObject>



QueueFrame::QueueFrame(QWidget *parent): QFrame(parent) {

	QVBoxLayout *vbox = new QVBoxLayout(this);

	toolbar = new QToolBar(this);
	vbox->addWidget(toolbar);

	actionStart = qRelightApp->addAction("queue_toolbar_start", "Start", "play", "");
	actionPause = qRelightApp->addAction("queue_toolbar_pause", "Pause", "pause", "");
	actionStop = qRelightApp->addAction("queue_toolbar_stop", "Stop", "square", "");

	toolbar->addAction(actionStart);
	toolbar->addAction(actionPause);
	toolbar->addAction(actionStop);

	connect(actionStart, &QAction::triggered, this, &QueueFrame::startQueue);
	connect(actionPause, &QAction::triggered, this, &QueueFrame::pauseQueue);
	connect(actionStop, &QAction::triggered, this, &QueueFrame::stopQueue);

	QLabel *activeLabel = new QLabel("Active Queue");
	vbox->addWidget(activeLabel);

	activeList = new QListWidget;
	activeList->setSelectionMode(QAbstractItemView::NoSelection);
	activeList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	activeList->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	vbox->addWidget(activeList);

	QLabel *historyLabel = new QLabel("History");
	vbox->addWidget(historyLabel);

	historyList = new QListWidget;
	historyList->setSelectionMode(QAbstractItemView::NoSelection);
	historyList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	historyList->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	vbox->addWidget(historyList);

	ProcessQueue &queue = ProcessQueue::instance();
	connect(&queue, SIGNAL(update()), this, SLOT(updateLists()));
	connect(&queue, SIGNAL(finished(Task *)), this, SLOT(taskFinished(Task *)));

	updateLists();
}

void QueueFrame::removeTask(Task *task) {
	ProcessQueue &queue = ProcessQueue::instance();
	queue.removeTask(task->id);
	updateLists();
}
void QueueFrame::startQueue() {
	ProcessQueue::instance().start();
}

void QueueFrame::pauseQueue() {
	ProcessQueue::instance().pause();
}

void QueueFrame::stopQueue() {
	ProcessQueue::instance().stop();
}

void QueueFrame::taskFinished(Task *task) {
	if(!task->visible)
		return;

	QJsonObject entry = task->info();
	entry.insert("log", task->log);
	entry.insert("error", task->error);

	qRelightApp->project().addCompletedTask(entry);
}

void QueueFrame::updateLists() {

	rebuildActiveList();
	rebuildHistoryList();

	ProcessQueue &queue = ProcessQueue::instance();
	if(actionStart)
		actionStart->setEnabled(queue.stopped || !queue.hasTasks());
	if(actionPause)
		actionPause->setEnabled(!queue.stopped);
	if(actionStop)
		actionStop->setEnabled(queue.task != nullptr);

}

void QueueFrame::rebuildActiveList() {
	ProcessQueue &queue = ProcessQueue::instance();
	QList<Task *> activeTasks;

	{
		QMutexLocker lock(&queue.lock);
		if(queue.task && queue.task->visible)
			activeTasks.append(queue.task);
		for(Task *task: queue.queue) {
			if(task->visible)
				activeTasks.append(task);
		}
	}

	activeList->clear();
	for(Task *task: activeTasks) {
		QueueItem *item = new QueueItem(task, activeList, false);
		activeList->addItem(item);
	}
}

void QueueFrame::rebuildHistoryList() {
	historyList->clear();
	historyEntries.clear();
	Project *project = qRelightApp->m_project;
	const QList<QJsonObject> &entries = project->taskHistory();
	for(const QJsonObject &entry: entries) {
		HistoryTask *task = new HistoryTask(entry);
		QueueItem *item = new QueueItem(task, historyList, true);
		historyList->addItem(item);
		historyEntries.push_back(task);
	}
}
