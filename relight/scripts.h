#ifndef SCRIPTS_H
#define SCRIPTS_H

#include <QStringList>
#include <QRect>
#include <vector>

#include <Eigen/Core>

class Scripts {
public:
	Scripts();
	static void deepzoom(QString plane, int quality = 98);       //expects "path/plane_1"
	static void tarzoom(QString plane);       //expects "path/plane_1"
	static void normals(QString output, QStringList images, const std::vector<Eigen::Vector3f> &lights, int method, QRect &crop);

	static bool checkModule(QString module);
private:
	static QString scriptDir();
};

#endif // SCRIPTS_H
