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
#include <QToolButton>
#include <QSize>



QueueFrame::QueueFrame(QWidget *parent): QFrame(parent) {

	QVBoxLayout *vbox = new QVBoxLayout(this);

	toolbar = new QToolBar(this);
	toolbar->setIconSize(QSize(18, 18));
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
	connect(&queue, SIGNAL(finished(QJsonObject)), this, SLOT(taskFinished(QJsonObject)));

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

void QueueFrame::taskFinished(QJsonObject task) {
	int status = task.value("status").toInt();
	if(status == Task::STOPPED)
		return;
	qRelightApp->project().addCompletedTask(task);
}

void QueueFrame::updateLists() {

	rebuildActiveList();
	rebuildHistoryList();

	ProcessQueue &queue = ProcessQueue::instance();
	ProcessQueue::State queueState = queue.state;
	if(actionStart)
		actionStart->setEnabled(queueState != ProcessQueue::RUNNING || !queue.hasTasks());
	if(actionPause)
		actionPause->setEnabled(queueState == ProcessQueue::RUNNING);
	if(actionStop)
		actionStop->setEnabled(queueState == ProcessQueue::RUNNING || queue.task != nullptr);

	updateToolbarState();

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

void QueueFrame::updateToolbarState() {
	ProcessQueue &queue = ProcessQueue::instance();
	ProcessQueue::State queueState = ProcessQueue::STOPPED;
	{
		QMutexLocker lock(&queue.lock);
		queueState = queue.state;
	}
	applyActionStyle(actionStart, "#328232", queueState == ProcessQueue::RUNNING);
	applyActionStyle(actionPause, "#828232", queueState == ProcessQueue::PAUSED);
	applyActionStyle(actionStop, "#823232", queueState == ProcessQueue::STOPPED);
}

void QueueFrame::applyActionStyle(QAction *action, const QString &color, bool highlight) {
	if(!toolbar || !action)
		return;
	if(QToolButton *button = qobject_cast<QToolButton *>(toolbar->widgetForAction(action))) {
		const QString baseStyle = QStringLiteral("QToolButton { padding:4px 10px; border-radius:4px; }");
		QString highlightStyle;
		if(highlight)
			highlightStyle = QString("QToolButton { background-color:%1; color:#b1b1b1; }").arg(color);
		button->setStyleSheet(baseStyle + highlightStyle);
		button->setIconSize(QSize(18, 18));
	}
}

void QueueFrame::rebuildHistoryList() {
	historyList->clear();
	// Delete old HistoryTask objects to avoid memory leak
	for(HistoryTask *task : historyEntries) {
		delete task;
	}
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
