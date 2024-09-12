#include "orixml.h"
#include <iostream>
#include <QtXml/QDomDocument>
#include <QFile>
#include <QDebug>
#include <QDomElement>
using namespace std;


OriXml::OriXml(QString path){

	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		throw QString("Cannot open file for reading: ")  + path;
		return;
	}

	if (!doc.setContent(&file)) {
		throw QString("Cannot parse XML file.");
		return;
	}

	QDomElement root = doc.documentElement();
	QDomElement centreElement = root.elementsByTagName("Centre").item(0).toElement();
	if (centreElement.isNull()) {
		throw QString("Centre element not found in XML.");
		return;
	}
	cout << qPrintable(centreElement.text()) << endl;
	//split, ritorna un Qstriglist
	//qlist

	QString centreText = centreElement.text();
	QStringList centreCoords = centreText.split(" ");
	if (centreCoords.size() != 3) {
		throw QString("Centre element does not contain 3 coordinates: ") + centreText;
	}
	center[0] = centreCoords[0].toDouble();

	rotationElementL1 = root.elementsByTagName("L1").item(0).toElement();
	rotationElementL2 = root.elementsByTagName("L2").item(0).toElement();
	rotationElementL3 = root.elementsByTagName("L3").item(0).toElement();

	if (rotationElementL1.isNull() || rotationElementL2.isNull() || rotationElementL3.isNull()) {
		throw QString("Rotation parameters not found in XML.");
	}
	QStringList L1 = rotationElementL1.text().split(" ");
	QStringList L2 = rotationElementL2.text().split(" ");
	QStringList L3 = rotationElementL3.text().split(" ");

	rotation << L1[0].toDouble(), L1[1].toDouble(), L1[2].toDouble(),
				L2[0].toDouble(), L2[1].toDouble(), L2[2].toDouble(),
				L3[0].toDouble(), L3[1].toDouble(), L3[2].toDouble();
	cout << "Centre (translation): ["
		 << center[0] << ", "
		 << center[1] << ", "
		 << center[2] << "]" << endl;

	cout << "Rotation Matrix:" << endl;
	cout << rotation << endl;
}
void OriXml::setOrientation(Eigen::Matrix3d r, Eigen::Vector3d c){

	rotation = r;
	center = c;
	// Centre Ca
	QString Ca = QString::number(c[0]) + " " + QString::number(c[1]) + " " + QString::number(c[2]);

	centreElement.firstChild().toText().setData(Ca);

	QString Ra1 = QString::number(r(0, 0)) + " " + QString::number(r(1, 0)) + " " + QString::number(r(2, 0));
	QString Ra2 = QString::number(r(0, 1)) + " " + QString::number(r(1, 1)) + " " + QString::number(r(2, 1));
	QString Ra3 = QString::number(r(0, 2)) + " " + QString::number(r(1, 2)) + " " + QString::number(r(2, 2));
	rotationElementL1.firstChild().toText().setData(Ra1);
	rotationElementL2.firstChild().toText().setData(Ra2);
	rotationElementL3.firstChild().toText().setData(Ra3);




}
void OriXml::saveOrientation(QString savePath) {
	QFile file(savePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		throw runtime_error("Failed to open the output file for writing.");
	}

	QTextStream stream(&file);
	stream << doc.toString();
	cout << qPrintable(doc.toString()) << endl;

	cout << "File saved correctly in " << savePath.toStdString() << endl;
}


	//definisci centro e matrice, utilizza eigen
	// /Users/erika/Desktop/Panorama_stone_2021-0-157/photogrammetry/Ori-Relative



