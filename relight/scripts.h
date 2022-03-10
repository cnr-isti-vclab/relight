#ifndef SCRIPTS_H
#define SCRIPTS_H

#include <QStringList>
#include <QRect>
#include <vector>
#include "../src/relight_vector.h"

namespace pybind11 {
	class object;
}

class Scripts {
public:
	Scripts();
	static void deepzoom(QString plane, int quality = 98);       //expects "path/plane_1"
	static void tarzoom(QString plane);       //expects "path/plane_1"
	static void normals(QString output, QStringList images, const std::vector<Vector3f> &lights, int method, QRect &crop);

	static bool checkModule(QString module);
private:
	static QString scriptDir();
};

#endif // SCRIPTS_H
