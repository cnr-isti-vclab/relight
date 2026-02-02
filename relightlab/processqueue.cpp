#include "processqueue.h"
#include "relightapp.h"
#include "historytask.h"

#include <QProcess>
#include <QDir>
#include <QSettings>
#include <QDateTime>

#include <iostream>
using namespace std;


ProcessQueue::~ProcessQueue() {
	Task *current = nullptr;
	{
		QMutexLocker locker(&lock);
		quitting = true;
		state = STOPPED;
		current = task;
	}
	if(current)
		current->stop();
	if(isRunning())
		wait();
}

void ProcessQueue::run() {
	while(true) {
		Task *currentTask = nullptr;
		{
			QMutexLocker locker(&lock);
			if(quitting)
				break;
			if(!task && state == RUNNING && !queue.isEmpty())
				startNewProcess();
		}

		if(!task) {
			msleep(50);
			continue;
		}

		task->wait(100);

		if(!task->isFinished())
			continue;

		if(task->visible) {
			QString msg = task->status == Task::DONE ? "Done" : task->error;
			msg = task->output + "\n" + msg;
			emit finished(task->info());
			emit finished(task->label, msg);
		}

		{
			QMutexLocker locker(&lock);
			if(!task->owned) {
				delete task;
			}
			task = nullptr;
		}
		emit update();
	}

}


/* mutexed! */
void ProcessQueue::startNewProcess() {
	task = queue.front();
	queue.pop_front();

	task->status = Task::RUNNING;

	task->start();
	emit update();
}

bool ProcessQueue::hasTasks() {
	QMutexLocker locker(&lock);
	if(task) return true;
	return queue.size() > 0;
}
bool ProcessQueue::contains(Task *a) {
	QMutexLocker locker(&lock);
	return a == task || queue.indexOf(a) >= 0 || past.indexOf(a) >= 0;

}


void ProcessQueue::addTask(Task *a, bool paused) {
	if(paused)
		a->pause();
	a->id = newId();
	{
		QMutexLocker locker(&lock);
		queue.push_back(a);
	}
	emit update();
}

void ProcessQueue::removeTask(Task *a) {
	{
		QMutexLocker locker(&lock);
		int index = past.indexOf(a);
		if(index >= 0) {
			past.takeAt(index);

		}
		index = queue.indexOf(a);
		if(index >= 0) {
			queue.takeAt(index);
		}
	}
	emit update();
}

void ProcessQueue::removeTask(int id) {
	{
		QMutexLocker locker(&lock);
		int index = indexOf(id);
		if(index < 0)
			return;

		queue.takeAt(index);
	}
	emit update();
}

void ProcessQueue::pushFront(int id) {
	{
		QMutexLocker locker(&lock);
		int index = indexOf(id);
		if(index < 0)
			return;

		Task *p = queue.takeAt(index);
		queue.push_front(p);
	}
	emit update();
}

void ProcessQueue::pushBack(int id) {
	{
		QMutexLocker locker(&lock);
		int index = indexOf(id);
		if(index < 0)
			return;

		Task *p = queue.takeAt(index);
		queue.push_back(p);
	}
	emit update();
}

void ProcessQueue::clear() {
	{
		QMutexLocker locker(&lock);
		queue.clear();
	}
	emit update();
}

void ProcessQueue::clearHistory() {
	QList<Task *> old;
	{
		QMutexLocker locker(&lock);
		old = past;
		past.clear();
	}
	for(Task *task: old) {
		if(!task->owned)
			delete task;
	}
	emit update();
}

void ProcessQueue::start() {
	bool shouldStartThread = false;
	{
		QMutexLocker locker(&lock);
		state = RUNNING;
		if(task)
			task->resume();
		shouldStartThread = !isRunning();
	}
	if(shouldStartThread)
		QThread::start();
	emit update();
}

void ProcessQueue::pause() {
	{
		QMutexLocker locker(&lock);
		state = PAUSED;
		if(task)
			task->pause();
	}
	emit update();
}

void ProcessQueue::stop() {
	Task *current = nullptr;
	{
		QMutexLocker locker(&lock);
		state = STOPPED;
		current = task;
	}
	if(current)
		current->stop();
	emit update();
}

int ProcessQueue::indexOf(int id) {
	for(int i = 0; i < queue.size(); i++) {
		int pid = queue[i]->id;
		if(pid == id)
			return i;
	}
	return -1;
}
