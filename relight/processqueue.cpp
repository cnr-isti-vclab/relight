#include "processqueue.h"
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

	task->start();
	task->status = Task::RUNNING;
	emit update();
/*	Script &script = process.script;
	QDir dir(process.script.dir);
	if(!dir.exists()) {
		process.error = "Could not find script folder: " + script.dir;
		process.failed = true;
	}

	QString script_path = dir.filePath(script.filename);
	QFileInfo info(script_path);
	if(!info.exists()) {
		process.error = "Could not the script: " + script_path;
		process.failed = true;
	}

	qprocess = new QProcess();
	qprocess->setProgram(script.interpreter);
	qprocess->setArguments(QStringList() << script_path << script.arguments());
	qprocess->start();
	//qprocess->waitForFinished(-1); */
}

void ProcessQueue::addTask(Task *a, bool paused) {
	if(paused)
		a->pause();
	a->id = newId();
	QMutexLocker locker(&lock);
	queue.push_back(a);
	emit update();
}

void ProcessQueue::removeTask(int id) {
	QMutexLocker locker(&lock);
	int index = indexOf(id);
	if(index >= 0) {
		Task *task = queue.takeAt(index);
		delete task;
	}
	emit update();
}

void ProcessQueue::pushFront(int id) {
	QMutexLocker locker(&lock);
	int index = indexOf(id);
	if(index < 0)
		return;

	Task *p = queue.takeAt(index);
	queue.push_front(p);
	emit update();
}

void ProcessQueue::pushBack(int id) {
	QMutexLocker locker(&lock);
	int index = indexOf(id);
	if(index < 0)
		return;

	Task *p = queue.takeAt(index);
	queue.push_back(p);
	emit update();
}

void ProcessQueue::clear() {
	QMutexLocker locker(&lock);
	queue.clear();
	emit update();
}

void ProcessQueue::start() {
	QMutexLocker locker(&lock);
	stopped = false;
	if(task)
		task->resume();
	if(!isRunning())
		QThread::start();
	emit update();
}

void ProcessQueue::pause() {
	QMutexLocker locker(&lock);
	stopped = true;
	if(task)
		task->pause();
	emit update();
}

void ProcessQueue::stop() {
	QMutexLocker locker(&lock);
	stopped = true;
	if(task)
		task->stop();
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
