#include "task.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QProcess>
#include <QDebug>

#include <iostream>
using namespace std;

void Task::runPythonScript(QString script, QStringList arguments, QString working) {
	QString python = QSettings().value("python_path").toString();
	if(python.isNull()) {
		error = "Python executable not set. See File -> Preferences -> Scripts.";
		status = FAILED;
		return;
	}
	runScript(python, script, arguments, working);
}

void Task::runScript(QString program, QString script, QStringList arguments, QString working) {
	QString scripts_path = QSettings().value("scripts_path").toString();
	
	if(scripts_path.isEmpty())
		QSettings().value("tmp_scripts_path").toString();
	
	QDir dir(scripts_path);
	if(!dir.exists()) {
		
		error = "Could not find script folder: " + dir.path();
		status = FAILED;
		return;
	}

	QString script_path = dir.filePath(script);
	QFileInfo info(script_path);
	if(!info.exists()) {
		error = "Could not find the script: " + script_path;
		status = FAILED;
	}


	QProcess process;
	if(!working.isNull())
		process.setWorkingDirectory(working);
	process.setProgram(program);
	process.setArguments(QStringList() << script_path << arguments);
	process.start();


	QRegExp re("([^:]+):\\ (\\d+)\\%");

	do {
		QString err = process.readAllStandardError();
		QString out = process.readAllStandardOutput();
		log.append(err);
		log.append(out);

		if(!out.isEmpty()) {
			QString line = out.split("\n").front();
			int pos = re.indexIn(line);

			if(pos >= 0) {
				QString text = re.capturedTexts()[1];
				int percent = re.capturedTexts()[2].toInt();
				emit progress(text, percent);
			}
		}
		if(err.size())
			qDebug() << "Err: " << qPrintable(err) << endl;
		if(out.size())
			qDebug() << "Out: " << qPrintable(out) << endl;
		if(status == PAUSED) {
			qDebug() << "DOn't know how to pause a process!" << endl;
		}
		if(status == STOPPED) {
			process.kill();
			process.waitForFinished();
			break;
		}
	} while(!process.waitForFinished(100));
	log.append(process.readAllStandardError());
	log.append(process.readAllStandardOutput());

	if(process.exitStatus() == QProcess::NormalExit) {
		status = DONE;
	} else
		status = FAILED;
	error = log;
}
