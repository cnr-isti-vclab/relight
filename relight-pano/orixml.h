#ifndef ORIXML_H
#define ORIXML_H
#include <QFile>
#include <QDir>
#include <QString>
#include <Eigen/Core>
#include <QDomDocument>

class OriXml {
public:
	OriXml(QString);
	QString filename;
	QDomDocument doc;
	QDomElement centreElement;
	QDomElement rotationElementL1, rotationElementL2, rotationElementL3;

	Eigen::Vector3d center;
	Eigen::Matrix3d rotation;

	void setOrientation(Eigen::Matrix3d r, Eigen::Vector3d t);
	void saveOrientation(QString savePath);



	//center Vector3double
	//rotation Matrix3d
	//QXmlDocument
	// void read filename
	// void write filename
	// coord change vector3drotation e vector3dtraslation
	//save file name join, set, text
};

#endif // ORIXML_H
