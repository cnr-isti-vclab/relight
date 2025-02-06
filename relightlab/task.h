#ifndef TASK_H
#define TASK_H

#include <QThread>
#include <QMutex>


class Task: public QThread {
	Q_OBJECT
public:
	int id = 0;
	QString label;
	bool visible = true; //visible on the queueframe.
	enum Mime { UNKNOWN, IMAGE, RELIGHT, RTI, PTM, MESH };
	Mime mime = UNKNOWN;

	QString input_folder;
	QString output;

	enum Status { ON_QUEUE = 0, RUNNING = 1, PAUSED = 2, STOPPED = 3, DONE = 4, FAILED = 5};
	Status status = ON_QUEUE;
	QString error;
	QString log;

	Task(QObject *parent = Q_NULLPTR): QThread(parent) {}
	virtual ~Task() {}
	virtual void stop();
	virtual void pause();
	virtual void resume();


	void runPythonScript(QString script, QStringList arguments, QString workingdir = QString());
	void runScript(QString program, QString script, QStringList arguments, QString workingdir = QString());
public slots:
	void test() {}
	virtual bool progressed(QString str, int percent);

signals:
	void progress(QString  str, int n);

protected:
	QMutex mutex;
};

#endif // TASK_H
