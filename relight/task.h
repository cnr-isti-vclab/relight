#ifndef TASK_H
#define TASK_H

#include <QThread>
#include <QMutex>

#include "parameter.h"


class Task: public QThread {
	Q_OBJECT
public:
	int id = 0;
	QString label;

	QString input_folder;
	QString output;
//	float estimated_time;  //in ms.

	QList<Parameter> parameters;

	enum Status { ON_QUEUE = 0, RUNNING = 1, PAUSED = 2, STOPPED = 3, DONE = 4, FAILED = 5};
	Status status = ON_QUEUE;
	QString error;
	QString log;

	Task(QObject *parent = Q_NULLPTR): QThread(parent) {}
	virtual ~Task() {}
    virtual void stop();
    virtual void pause();
    virtual void resume();

	void addParameter(const QString &id, Parameter::Type type, QVariant value) {
		parameters.append(Parameter(id, type, value));
	}
	bool hasParameter(const QString &id) {
		for(Parameter &p: parameters)
			if(p.id == id)
				return true;
		return false;
	}
	Parameter &operator[](const QString &id) {
		for(Parameter &p: parameters)
			if(p.id == id)
				return p;
		throw QString("Missing parameter: " + id);
	}

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
