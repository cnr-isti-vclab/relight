#include "orixml.h"
#include <iostream>
#include <QtXml/QDomDocument>
#include <QFile>
#include <QDebug>
#include <QDomElement>
using namespace std;


OriXml::OriXml(QString){
	QString xmlFilePath = "/Users/erika/Desktop/Panorama_stone_2021-0-157/photogrammetry/Ori-Relative/Orientation-Face_A.JPG.xml";

	QFile file(xmlFilePath);
	if (!file.open(QIODevice::ReadOnly)) {
		throw QString("Cannot open file for reading: ")  + xmlFilePath;
		return;
	}

	QDomDocument doc;
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


	QDomElement rotationElementL1 = root.elementsByTagName("L1").item(0).toElement();
	if (rotationElementL1.isNull()) {
		throw QString("Rotation parameters not found in XML.");
	}
	cout << qPrintable(rotationElement.text());

}



	//definisci centro e matrice, utilizza eigen
	// /Users/erika/Desktop/Panorama_stone_2021-0-157/photogrammetry/Ori-Relative



