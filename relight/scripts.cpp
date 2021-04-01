//this include must be the first one, not to be messed by slots keyword defined by QT.
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/numpy.h>
namespace py = pybind11;

#include <QFile>

#include <iostream>
using namespace std;


#include "scripts.h"

Scripts::Scripts() {}

void Scripts::deepzoom(QString plane, int quality) {
	py::scoped_interpreter guard{};

	py::object scope = py::module_::import("__main__").attr("__dict__");
	scope["plane"] = plane.toStdString();
	scope["quality"] = quality;
	Scripts::runScript("deepzoom.py", scope);
}

void Scripts::tarzoom(QString plane) {
	py::scoped_interpreter guard{};

	py::object scope = py::module_::import("__main__").attr("__dict__");
	scope["plane"] = plane.toStdString();
	Scripts::runScript("tarzoom.py", scope);
}

void Scripts::normals(QString output, QStringList images, const std::vector<Vector3f> &lights, int method) {
	py::scoped_interpreter guard{};

	py::object scope = py::module_::import("__main__").attr("__dict__");

	try {
		scope["output"] = output.toStdString();

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

	QStringList scripts;
	scripts << "psutils.py" << "rpsnumerics.py" << "rps.py";
	Scripts::runScripts(scripts, scope);

}


void Scripts::runScripts(QStringList scripts, py::object &scope) {
	QByteArray buffer;
	for(QString filename: scripts) {
		QFile file("/home/ponchio/devel/relight/scripts/" + filename);
		file.open(QFile::ReadOnly);
		buffer.append(file.readAll());
//		mModule = py::module::import("module_name");

	}
	Scripts::run(buffer.data(), scope);
}

void Scripts::runScript(QString script, py::object &scope) {
	QFile file("/home/ponchio/devel/relight/scripts/" + script);
	file.open(QFile::ReadOnly);
	Scripts::run(file.readAll().data(), scope);
}


void Scripts::run(const char *data, py::object &scope) {
	try {
		py::exec(data, scope);

	} catch(py::error_already_set &e) {

		cout << qPrintable(data) << ": " << std::string(py::str(e.type())) << endl;
		cout << std::string(py::str(e.value())) << endl;

	} catch(...) {
		cout << qPrintable(data) << ": Something wrong!" << endl;
	}
}
