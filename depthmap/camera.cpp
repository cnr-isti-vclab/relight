#include "camera.h"
#include <tiffio.h>
#include <iostream>
#include <QImage>
#include <QDir>
#include <QDomElement>
#include <QtXml/QDomDocument>
#include "depthmap.h"
#include "../src/bni_normal_integration.h"
#include <QFile>
#include <QDebug>
#include <QRegularExpression>
#include <fstream>

using namespace std;

bool Camera::loadXml(const QString &pathXml){
	//orientation xml
	QFile file(pathXml);
	if (!file.open(QIODevice::ReadOnly)) {
		cerr << "Cannot open XML file: " << pathXml.toStdString() << endl;
		return false;
	}

	QDomDocument doc;
	doc.setContent(&file);
	QDomElement root = doc.documentElement();

	QDomNodeList matrixNodes = doc.elementsByTagName("CodageMatr").at(0).childNodes();
	for (int i = 0; i < 3; ++i) {
		QDomElement rowElement = matrixNodes.at(i).toElement();
		QStringList values = rowElement.text().split(" ");

		if (values.size() >= 3) {
			for (int j = 0; j < 3; ++j) {
				rotation(i, j) = values.at(j).toFloat();
			}
		} else {
			cerr << "Not enough values in row " << i << endl;
		}
	}

	QDomNodeList centreNodes = doc.elementsByTagName("Centre");
	if (!centreNodes.isEmpty()) {
		QStringList centreValues = centreNodes.at(0).toElement().text().split(" ");
		center[0] = centreValues.at(0).toFloat();
		center[1] = centreValues.at(1).toFloat();
		center[2] = centreValues.at(2).toFloat();
	} else {
		cerr << "Centre not found in XML." << endl;
	}
	QDomElement ExportAPERO = doc.firstChildElement("ExportAPERO");
	if (ExportAPERO.isNull()) {
		cerr << "ExportAPERO missing." << endl;
		return false;
	}

	QDomElement conique = ExportAPERO.firstChildElement("OrientationConique");
	if (conique.isNull()) {
		cerr << "OrientationConique missing." << endl;
		return false;
	}


	QDomElement fileInterneNode = conique.firstChildElement("FileInterne");
	if (fileInterneNode.isNull()) {
		cerr << "FileInterne missing." << endl;
		return false;
	}
	QString internePath = fileInterneNode.text();
	QFileInfo fileInfo(pathXml);
	QString dirPath = fileInfo.path();
	QDir dir(dirPath);
	dir.cd("..");

	//QString interneFileName = "AutoCal_Foc-60000_Cam-NIKON_D800E.xml";
	QString fullInternePath = dir.absoluteFilePath(internePath);
	if (!loadInternParameters(fullInternePath)) {
		qDebug() << "Error to download the parameters to:" << fullInternePath;
		return false;
	}

	return true;
	//autocal_foc.xml

}


bool Camera::loadInternParameters(const QString &internePath){

	QFile interneFile(internePath);
	if (!interneFile.open(QIODevice::ReadOnly)) {
		cerr << "Cannot open internal XML file: " << internePath.toStdString() << endl;
	}
	QDomDocument interneDoc;
	interneDoc.setContent(&interneFile);
	QDomElement interneRoot = interneDoc.documentElement();
	QDomElement calibNode = interneRoot.firstChildElement("CalibrationInternConique");

	QDomElement focalElement = calibNode.firstChildElement("F");
	if (!focalElement.isNull())
		focal = focalElement.text().toFloat();
	else{
		cerr << "No 'F' node found in internal XML" << endl;
		return false;
	}

	QDomElement sizeImg = calibNode.firstChildElement("SzIm");
	if (!sizeImg.isNull()) {
		QStringList dimensions = sizeImg.text().split(" ");
		if (dimensions.size() == 2){
			width = dimensions[0].toUInt();
			height = dimensions[1].toUInt();
		} else {
			cerr << "No 'SzIm' node found in internal XML" << endl;
			return false;
		}
	}
	QDomElement ppElement = calibNode.firstChildElement("PP");
	if (!ppElement.isNull()) {
		QStringList ppValues = ppElement.text().split(" ");
		if (ppValues.size() == 2){
			PPx = ppValues[0].toFloat();
			PPy = ppValues[1].toFloat();
		} else{
			cerr << "No 'PP' node found in internal XML" << endl;
			return false;
		}
	}
	QDomElement distortionNode = calibNode.firstChildElement("CalibDistortion").firstChildElement("ModRad");

	QDomElement cDistElement = distortionNode.firstChildElement("CDist");
	if (!cDistElement.isNull()) {
		QStringList cDistValues = cDistElement.text().split(" ");
		if (cDistValues.size() == 2) {
			Cx = cDistValues[0].toFloat();
			Cy = cDistValues[1].toFloat();
		} else {
			cerr << "No 'CDist' node found in internal XML." << endl;
			return false;
		}
	}

	QDomNodeList coeffDistNodes = distortionNode.elementsByTagName("CoeffDist");
	if (coeffDistNodes.size() >= 1)
		R3 = coeffDistNodes.at(0).toElement().text().toFloat();
	if (coeffDistNodes.size() >= 2)
		R5 = coeffDistNodes.at(1).toElement().text().toFloat();
	if (coeffDistNodes.size() >= 3)
		R7 = coeffDistNodes.at(2).toElement().text().toFloat();

	return true;

}

// Pc = Rk(Pg − Ok)
// Pg = Ground point Pc = point camera. x y z orientati come la camera, moltiplica la matrice. Poi fai la proiezione.
Eigen::Vector3f Camera::projectionToImage(Eigen::Vector3f realPosition) const{
	//centre origine
	//r matrice
	//matrice r inversa rotation
	Eigen::Matrix3f rotationInverse = rotation.transpose();
	Eigen::Vector3f cameraCoords = rotationInverse * (realPosition - center);

	//proiezione
	if (cameraCoords.z() == 0) {
		cerr << "Z è zero, impossibile proiettare il punto." << endl;
		return Eigen::Vector3f(0, 0, 0);
	}
	//Normalize by dividing by the z coordinate to get the image coordinates u and v as projected 2D coordinates
	float u = cameraCoords.x() / cameraCoords.z();
	float v = cameraCoords.y() / cameraCoords.z();
	//u, v cerca manuale micmac focale
	float z = cameraCoords.z();
	//
	Eigen::Vector3f uvz (u, v, z);
	uvz = applyIntrinsicCalibration(uvz);
	//uvz = applyRadialDistortion(uvz);
	return uvz;

}

Eigen::Vector3f Camera::projectionToReal(Eigen::Vector3f imgPosition) const{
	Eigen::Vector3f uvz = imgPosition;
	float u = (uvz.x() - PPx) / focal;
	float v = (uvz.y() - PPy) / focal;
	float z = uvz.z();
	Eigen::Vector3f cameraCoords(u * z, v * z, z);
	//world to camera projection
	Eigen::Vector3f worldPoint = rotation * cameraCoords + center;

	return worldPoint;


}


Eigen::Vector3f Camera::applyIntrinsicCalibration(Eigen::Vector3f& uvz) const{

	float u = uvz.x();
	float v = uvz.y();
	Eigen::Vector2f P(PPx, PPy);
	Eigen::Vector2f UV(u, v);
	Eigen::Vector2f result = P + focal * UV;

	return Eigen::Vector3f(result.x(), result.y(), uvz.z());
}
// du =U−Cx dv =V −Cy ρ2 =d2u +d2v


Eigen::Vector3f Camera::applyRadialDistortion(Eigen::Vector3f& uvz) {

	float du = uvz.x() - Cx;
	float dv = uvz.y() - Cy;
	float rho2 = du * du + dv * dv;

	float distortionFactor = 1 + R3 * rho2 + R5 * std::pow(rho2, 2) + R7 * std::pow(rho2, 3);

	float u_dist = Cx + distortionFactor * du;
	float v_dist = Cy + distortionFactor * dv;

	return Eigen::Vector3f(u_dist, v_dist, uvz.z());
}
void Camera::scale(int w, int h){

	float fx = float(w) / float(width);
	float fy = float(h) / float(height);
	float f = sqrt(fx * fy);
	focal *= f;
	PPx *=f;
	PPy *= f;
	Cx *= f;
	Cy *= f;
	width = w;
	height = h;

}



