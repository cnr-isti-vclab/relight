#include "script.h"

#include <QDebug>
#include <QSettings>
#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include <QTemporaryFile>

#include <iostream>
using namespace std;

Script::~Script() {
	for(auto file: tmp_files)
		delete file;
}

QStringList Script::arguments() {
	QStringList list;
	for(Parameter &p: parameters) {
		list.append(p.arguments());
		if(p.type == Parameter::TMP_FILE) {
			QTemporaryFile *file = new QTemporaryFile;
			file->open();
			file->write(p.value.toByteArray());
			file->flush();
			list << file->fileName();
			tmp_files.push_back(file);
		}
	}
	return list;
}

void Script::run() {
	
	QString scripts_path = QSettings().value("scripts_path").toString();
	
	if(scripts_path.isEmpty())
		scripts_path = QSettings().value("tmp_scripts_path").toString();
	
	
	QDir dir(scripts_path);
	if(!dir.exists()) {
		error = "Could not find script folder: " + dir.path();
		status = FAILED;
		return;
	}

	QString python = QSettings().value("python_path").toString();
	if(python.isNull()) {
		error = "Python executable not set";
		status = FAILED;
		return;
	}


	QString script_path = dir.filePath(script_filename);
	QFileInfo info(script_path);
	if(!info.exists()) {
		error = "Could not find the script: " + script_path;
		status = FAILED;
	}

	QProcess process;
	process.setProgram(python);
	qDebug() << scripts_path << script_path << arguments();
	process.setArguments(QStringList() << script_path << arguments());
	process.start();


	QRegExp re("([^:]+):\\ (\\d+)\\%");

	QString log;
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
			cout << "Err: " << qPrintable(err) << endl;
		if(out.size())
			cout << "Out: " << qPrintable(out) << endl;
		if(status == PAUSED) {
			cout << "DOn't know how to pause a process!" << endl;
		}
		if(status == STOPPED) {
			process.kill();
			process.waitForFinished();
			break;
		}
	} while(!process.waitForFinished(10));
	log.append(process.readAllStandardError());
	log.append(process.readAllStandardOutput());

	if(process.exitStatus() == QProcess::NormalExit) {
		status = DONE;
	} else
		status = FAILED;
}

void Script::pause() {
	//cant' really
}

void Script::resume() {
	//can't really
}

void Script::stop() {
	status = STOPPED;
}
