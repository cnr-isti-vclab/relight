#include <QFile>
#include <QFileDialog>
#include <QSettings>
#include <QProcess>

#include <iostream>
using namespace std;


#include "scripts.h"

Scripts::Scripts() {}

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

void Scripts::normals(QString output, QStringList images, const std::vector<Vector3f> &lights, int method) {
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

