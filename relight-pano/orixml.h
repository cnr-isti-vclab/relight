#ifndef ORIXML_H
#define ORIXML_H
#include <QFile>
#include <QDir>
#include <QString>
#include <Eigen/Core>

class OriXml {
public:
	OriXml(QString);
	QString filename;

	Eigen::Vector3d center;

	//center Vector3double
	//rotation Matrix3d
	//QXmlDocument
	// void read filename
	// void write filename
	// coord change vector3drotation e vector3dtraslation
};

#endif // ORIXML_H
