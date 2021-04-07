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
	QSettings settings;
	QDir dir(settings.value("scripts_path").toString());
	if(!dir.exists()) {
		error = "Could not find script folder: " + dir.path();
		status = FAILED;
	}

	QString script_path = dir.filePath(script_filename);
	QFileInfo info(script_path);
	if(!info.exists()) {
		error = "Could not find the script: " + script_path;
		status = FAILED;
	}

	QProcess process;
	process.setProgram(interpreter);
	qDebug() << arguments();
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
		status = FAILED;
	} else
		status = DONE;
}

void Script::pause() {

}

void Script::resume() {

}

void Script::stop() {

}

/*Script Script::Normals() {

	QSettings settings;;
	Script script;
	script.interpreter = "python3"; //todo read from script with full path!
	script.dir = settings.value("scripts_path").toString();
	script.filename = "normals/normalmap.py";
	script.id = "Normals";
	script.label = "Normals";
	script.hint = "Compute normals";

	script.parameters.append(Parameter("path", Parameter::FOLDER));
	script.parameters.append(Parameter("output", Parameter::FOLDER));
	script.parameters.append(Parameter("images", Parameter::STRINGLIST));
	script.parameters.append(Parameter("lights", Parameter::DOUBLELIST));

	Parameter method("method", Parameter::INT);
	method.domain["Least squares"] = 0;
	method.domain["Baesian"] = 4;
	method.domain["Robust PCA"] = 5;
	script.parameters.append(method);

	script.parameters.append(Parameter("crop", Parameter::RECT));
	return script;
}

Script Script::RTI() {
	Script script;
	script.id = "RTI";
	script.label = "RTI";
	script.hint = "Compute an RTI";

	script.parameters.append(Parameter("path", Parameter::FOLDER));
	script.parameters.append(Parameter("output", Parameter::FOLDER));
	script.parameters.append(Parameter("images", Parameter::STRINGLIST));
	script.parameters.append(Parameter("lights", Parameter::DOUBLELIST));

	//TODO need a domain as a label -> value
	Parameter type("type", Parameter::INT);
	type.domain["PTM"] = 0;
	type.domain["HSH"] = 1;
	type.domain["RBF"] = 2;
	type.domain["Bilinear"] = 3;
	script.parameters.append(type);

	Parameter colorspace("colorspace", Parameter::INT);
	colorspace.domain["RGB"] = 0;
	colorspace.domain["LRGB"] = 1;
	colorspace.domain["YCC"] = 2;
	colorspace.domain["MRGB"] = 3;
	colorspace.domain["MYCC"] = 4;
	script.parameters.append(colorspace);

	Parameter nplanes("nplanes", Parameter::INT);
	nplanes.min = 6;
	nplanes.max = 36;
	script.parameters.append(nplanes);

	Parameter yplanes("yplanes", Parameter::INT);
	nplanes.min = 6;
	nplanes.max = 36;
	script.parameters.append(yplanes);

	script.parameters.append(Parameter("crop", Parameter::RECT));

	Parameter ram("ram", Parameter::INT);
	ram.min = 128;
	ram.max = 12000;
	script.parameters.append(ram);

	return script;
} */
