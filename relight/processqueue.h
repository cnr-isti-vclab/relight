#ifndef PROCESSQUEUE_H
#define PROCESSQUEUE_H


#include <QString>
#include <QVariant>
#include <QList>
#include <QThread>
#include <QMutex>
#include <QTemporaryFile>

#include "task.h"

#include <iostream>
using namespace std;


class ProcessQueue: public QThread {
	Q_OBJECT
public:
	~ProcessQueue();
	int nthreads = 4;

	Task *task = nullptr; //task is removed from the queue when executing.
	bool stopped = false;

	QList<Task *> queue;
	QList<Task *> past;
	QMutex lock;

	static ProcessQueue &instance() {
		static ProcessQueue single;
		return single;
	}

	bool hasTasks();
	void addTask(Task *a, bool paused = false);
	void removeTask(Task *a);
	void removeTask(int id);
	void pushFront(int id);
	void pushBack(int id);
	void clear();

	void start(); //start the queue if not start
	void pause(); //pause current process.
	void stop(); //stop cuirrent process and pause the queue.

public slots:

signals:
	void update(); //a task was added, or started, or finished.

protected:
	void run();
	void startNewProcess();
	int indexOf(int id);
	int newId() {
		static int id = 0;
		return id++;
	}
};

#endif // PROCESSQUEUE_H
