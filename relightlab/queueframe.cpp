#include "queueframe.h"

#include "processqueue.h"
#include "queueitem.h"
#include "relightapp.h"


#include <QVBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QMutexLocker>
#include <QToolBar>
#include <QAction>



QueueFrame::QueueFrame(QWidget *parent): QFrame(parent) {

	QVBoxLayout *vbox = new QVBoxLayout(this);

	toolbar = new QToolBar(this);
	vbox->addWidget(toolbar);

	actionStart = qRelightApp->addAction("queue_toolbar_start", tr("Start"), "play", "");
	actionPause = qRelightApp->addAction("queue_toolbar_pause", tr("Pause"), "pause", "");
	actionStop = qRelightApp->addAction("queue_toolbar_stop", tr("Stop"), "stop", "");

	toolbar->addAction(actionStart);
	toolbar->addAction(actionPause);
	toolbar->addAction(actionStop);

	connect(actionStart, &QAction::triggered, this, &QueueFrame::startQueue);
	connect(actionPause, &QAction::triggered, this, &QueueFrame::pauseQueue);
	connect(actionStop, &QAction::triggered, this, &QueueFrame::stopQueue);

	QLabel *activeLabel = new QLabel(tr("Active Queue"));
	activeLabel->setObjectName("queueActiveLabel");
	vbox->addWidget(activeLabel);

	activeList = new QListWidget;
	activeList->setObjectName("queueActiveList");
	activeList->setSelectionMode(QAbstractItemView::NoSelection);
	vbox->addWidget(activeList);

	QLabel *historyLabel = new QLabel(tr("History"));
	historyLabel->setObjectName("queueHistoryLabel");
	vbox->addWidget(historyLabel);

	historyList = new QListWidget;
	historyList->setObjectName("queueHistoryList");
	historyList->setSelectionMode(QAbstractItemView::NoSelection);
	vbox->addWidget(historyList);

	ProcessQueue &queue = ProcessQueue::instance();
	connect(&queue, SIGNAL(update()), this, SLOT(update()));

	update();
}

void QueueFrame::removeTask(Task *task) {
	ProcessQueue &queue = ProcessQueue::instance();
	queue.removeTask(task->id);
	update();
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
void QueueFrame::update() {
	ProcessQueue &queue = ProcessQueue::instance();

	QList<Task *> activeTasks;
	QList<Task *> historyTasks;

	{
		QMutexLocker lock(&queue.lock);
		if(queue.task && queue.task->visible)
			activeTasks.append(queue.task);
		for(Task *task: queue.queue) {
			if(task->visible)
				activeTasks.append(task);
		}
		for(Task *task: queue.past) {
			if(task->visible)
				historyTasks.append(task);
		}
	}

	rebuildActiveList(activeTasks);
	rebuildHistoryList(historyTasks);

	if(actionStart)
		actionStart->setEnabled(queue.stopped || !queue.hasTasks());
	if(actionPause)
		actionPause->setEnabled(!queue.stopped);
	if(actionStop)
		actionStop->setEnabled(queue.task != nullptr);

}

void QueueFrame::rebuildActiveList(const QList<Task *> &tasks) {
	activeList->clear();
	for(Task *task: tasks) {
		QueueItem *item = new QueueItem(task, activeList, false);
		activeList->addItem(item);
	}
}

void QueueFrame::rebuildHistoryList(const QList<Task *> &tasks) {
	historyList->clear();
	for(Task *task: tasks) {
		QueueItem *item = new QueueItem(task, historyList, true);
		historyList->addItem(item);
	}
}
