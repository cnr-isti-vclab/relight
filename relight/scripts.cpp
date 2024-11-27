#include "scripts.h"

#include <QFile>
#include <QFileDialog>
#include <QSettings>
#include <QTemporaryFile>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include <iostream>

using namespace std;
using namespace Eigen;




Scripts::Scripts() {}


bool Scripts::checkModule(QString module) {


	QProcess process;
	process.setProgram("/usr/bin/python3");
	process.setArguments(QStringList()
		<< "-c"
		<< QString("import pkgutil; return 1 if pkgutil.find_loader('%1') else 0)").arg(module));
	process.start();
	process.waitForFinished(-1);
	int out = process.exitCode();

	return out;
}


void Scripts::deepzoom(QString plane, int quality) {

	QString dir = scriptDir();

	QString command = QString("/usr/bin/python3 %1/deepzoom.py \"%2\" %3").arg(dir).arg(plane).arg(quality);
	cout << "Command: " << qPrintable(command) << endl;
	if(QProcess::execute(command) < 0)
		cout << "Failed!" << endl;
}

void Scripts::tarzoom(QString plane) {

	QString dir = scriptDir();

	QString command = QString("/usr/bin/python3 %1/tarzoom.py \"%2\"").arg(dir).arg(plane);
	cout << "Command: " << qPrintable(command) << endl;
	if(QProcess::execute(command) < 0)
		cout << "Failed!" << endl;
}


void Scripts::normals(QString output, QStringList images, const std::vector<Vector3f> &lights, int method, QRect &crop) {

	QStringList modules;
	modules << "glob" << "ast" << "json" << "numpy" << "sklearn" << "pickle" << "PIL";
	for(auto module: modules) {
		int ok = checkModule(module);
		if(!ok)
			return;
	}

	QJsonArray jlights;
	for(auto light: lights) {
		jlights.push_back(QJsonArray() << light[0] << light[1] << light[2]);
	}
	QJsonObject obj;
	obj["lights"] =  jlights;

	QJsonArray jimages;
	for(auto image: images) {
		jimages.push_back(image);
	}
	obj["images"] = jimages;

	if(crop.isValid()) {
		QJsonObject c;
		c["x"] = crop.left();
		c["y"] = crop.top();
		c["width"] = crop.width();
		c["height"] = crop.height();
		obj["crop"] = c;
	}

	QJsonDocument doc;
	doc.setObject(obj);

	QTemporaryFile lp;
	lp.open();
	lp.write(doc.toJson());
	lp.flush();
	lp.fileName();

	QString dir = scriptDir();

	QProcess process;
	process.setProgram("/usr/bin/python3");
	process.setArguments(QStringList()
		<< QString("%1/normals/normalmap.py").arg(dir)
		<< lp.fileName()
		<< output
		<< QString::number(method));

	process.start();
	do {
		QString err = process.readAllStandardError();
		QString out = process.readAllStandardOutput();
		if(err.size())
			cout << qPrintable(err) << endl;
		if(out.size())
			cout << qPrintable(out) << endl;
	} while(!process.waitForFinished(1000));
	cout << qPrintable(process.readAllStandardError()) << endl;
	cout << qPrintable(process.readAllStandardOutput()) << endl;
	if(process.exitStatus() != QProcess::NormalExit)
		cout << "Failed!" << endl;

	/*py::scoped_interpreter guard{};

	py::object scope = py::module_::import("__main__").attr("__dict__");

	try {
		scope["output"] = output.toStdString();

		py::exec("import numpy", scope);
		return;
		using namespace pybind11::literals;
		py::array_t<float> pylights({3, int(lights.size())});
		memcpy(pylights.mutable_data(), lights.data(), lights.size()*4);

		scope["lights"] = pylights;


	} catch(py::error_already_set &e) {

		cout << "Normals: " << std::string(py::str(e.type())) << endl;
		cout << std::string(py::str(e.value())) << endl;

	} catch(...) {
		cout << qPrintable("Normals") << ": Something wrong!" << endl;
	}

	Scripts::runScript("normals/normalmap.py", scope);
 */
}


QString Scripts::scriptDir() {
	QSettings settings;
	QString dir = settings.value("scripts_path").toString();
	if(dir.isNull()) {
		dir = QFileDialog::getExistingDirectory(nullptr, "Choose picture folder");
		if(dir.isNull())
			return dir;
		settings.setValue("scripts_path", dir);
	}
	return dir;
}

