#ifndef SCRIPTS_H
#define SCRIPTS_H

#include <QStringList>
#include <vector>
#include "../src/vector.h"

namespace pybind11 {
	class object;
}

class Scripts {
public:
	Scripts();
	static void deepzoom(QString plane, int quality = 98);       //expects "path/plane_1"
	static void tarzoom(QString plane);       //expects "path/plane_1"
	static void normals(QString output, QStringList images, const std::vector<Vector3f> &lights, int method);

private:
	static void run(const char *data, pybind11::object &scope);
	static void runScript(QString script, pybind11::object &scope);
	static void runScripts(QStringList script, pybind11::object &scope);
};

#endif // SCRIPTS_H
