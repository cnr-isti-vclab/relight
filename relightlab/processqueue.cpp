#include "processqueue.h"
#include "relightapp.h"

#include <QProcess>
#include <QDir>
#include <QSettings>

#include <iostream>
using namespace std;


ProcessQueue::~ProcessQueue() {
	if(!isRunning())
		return;
	{
		QMutexLocker locker(&lock);
		stopped = true;
	}

	wait();
}

void ProcessQueue::run() {
	while(1) {
		lock.lock();
		if(stopped) {
			lock.unlock();
			break;
		}


		if(!task) {
			if(queue.size() == 0) {
				lock.unlock();
				sleep(1);
			} else {
				startNewProcess();
				lock.unlock();
			}
			continue;
		}
		lock.unlock();

		task->wait(100);

		if(task->isFinished()) {
			if(task->visible) {
				QString msg = task->status == Task::DONE ? "Done" : task->error;
				msg = task->output + "\n" + msg;
				emit finished(task->label, msg);
			}
			if(!task->owned)
				past.push_back(task);
			task = nullptr;
			emit update();
		}
	}

}


/* mutexed! */
void ProcessQueue::startNewProcess() {
	task = queue.front();
	queue.pop_front();

	//task->mutex.lock();
	task->status = Task::RUNNING;
	//task->mutex.unlock();

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
		//processqueue is never the owner!
		if(!task->owned)
			delete task;
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

void ProcessQueue::start() {
	{
		QMutexLocker locker(&lock);
		stopped = false;
		if(task)
			task->resume();
		if(!isRunning())
			QThread::start();
	}
	emit update();
}

void ProcessQueue::pause() {
	{
		QMutexLocker locker(&lock);
		stopped = true;
		if(task)
			task->pause();
	}
	emit update();
}

void ProcessQueue::stop() {
	{
		QMutexLocker locker(&lock);
		stopped = true;
		if(task)
			task->stop();
	}
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
