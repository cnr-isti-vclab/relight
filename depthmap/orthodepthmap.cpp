#include "orthodepthmap.h"
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
#include "gaussiangrid.h"

using namespace std;
//start OrthoDepthMap class
// MicMac depth and features

bool OrthoDepthmap::loadXml(const char *xmlPath){
	//depthmap in Malt Z deZoom ecc
	QFile file(xmlPath);
	if (!file.open(QIODevice::ReadOnly)) {
		cerr << "Cannot open XML file: " << xmlPath << endl;
		return false;
	}

	QDomDocument doc;
	doc.setContent(&file);

	QDomElement root = doc.documentElement();
	QDomNodeList originePlaniNodes = root.elementsByTagName("OriginePlani");
	QDomNodeList resolutionPlaniNodes = root.elementsByTagName("ResolutionPlani");
	QDomNodeList origineAltiNodes = root.elementsByTagName("OrigineAlti");
	QDomNodeList resolutionAltiNodes = root.elementsByTagName("ResolutionAlti");


	if (originePlaniNodes.isEmpty() || resolutionPlaniNodes.isEmpty() ||
		origineAltiNodes.isEmpty() || resolutionAltiNodes.isEmpty()) {
		cerr << "OriginePlani, ResolutionPlani, OrigineAlti, or ResolutionAlti not found in XML." << endl;
		return false;

	}

	//  <OriginePlani>-2.72 3.04</OriginePlani>
	QStringList origineValues = originePlaniNodes.at(0).toElement().text().split(" ");
	if (origineValues.size() >= 2) {
		origin[0] = origineValues.at(0).toFloat();
		origin[1] = origineValues.at(1).toFloat();
	}
	QStringList resolutionValues = resolutionPlaniNodes.at(0).toElement().text().split(" ");
	if (resolutionValues.size() >= 2) {
		resolution[0] = resolutionValues.at(0).toFloat();
		resolution[1] = resolutionValues.at(1).toFloat();
	}

	// <ResolutionPlani>0.128 -0.128</ResolutionPlani> passo

	//resAlti e oriAlti
	origin[2] = origineAltiNodes.at(0).toElement().text().toFloat();
	resolution[2] = resolutionAltiNodes.at(0).toElement().text().toFloat();

	return true;

}

bool OrthoDepthmap::load(const char *depth_path, const char *mask_path){

	QString qdepth_path = QString(depth_path);
	if(!loadDepth(qdepth_path.toStdString().c_str())){
		cerr << "Failed to load ortho depth tiff file: " << qdepth_path.toStdString() << endl;
		return false;
	}

	QString qmask_path = QString(mask_path);
	if(!loadMask(qmask_path.toStdString().c_str())){
		cerr << "Failed to load ortho mask tiff file: " << qmask_path.toStdString() << endl;
		return false;
	}

	QString xmlPath = qdepth_path.left(qdepth_path.lastIndexOf('.')) + ".xml";
	if (!loadXml(xmlPath.toStdString().c_str())) {
		cerr << "Failed to load XML file: " << xmlPath.toStdString() << endl;
		return false;
	}
	return true;

}
Eigen::Vector3f OrthoDepthmap::pixelToRealCoordinates(int pixelX, int pixelY, float pixelZ) {

	// converto in punti 3d. origine dell'img + passoX * 160x
	float realX = origin[0] + resolution[0] * pixelX;
	float realY = origin[1] + resolution[1] * pixelY;
	float realZ = origin[2] + resolution[2] * pixelZ;

	return Eigen::Vector3f(realX, realY, realZ);
}


void OrthoDepthmap::saveObj(const char *filename){
	// use QFile for write the file and after QTextStream
	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		qDebug() << "Cannot open file for writing:" << filename;
		return;
	}
	QTextStream out(&file);

	for (uint32_t y = 0; y < height; y++) {
		for (uint32_t x = 0; x < width; x++) {
			float z = elevation[x + y * width];
			Eigen::Vector3f realPos = pixelToRealCoordinates(x, y, z);
			//obj coordinates of a point v first string v second string etc. and then exit call in main
			out << "v " << realPos.x() << " " << realPos.y() << " " << realPos.z() << "\n";
		}
	}
}
void OrthoDepthmap::projectToCameraDepthMap(const Camera& camera, const QString& outputPath) {

	QImage depthMapImage(camera.width, camera.height, QImage::Format_RGB888);
	depthMapImage.fill(qRgb(0, 0, 0));
	//find the minimum and maximum for the Z coordinates
	float minZ = std::numeric_limits<float>::max();
	float maxZ = std::numeric_limits<float>::lowest();
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			float pixelZ = elevation[x + y * width];

			Eigen::Vector3f realCoordinates = pixelToRealCoordinates(x, y, pixelZ);
			Eigen::Vector3f imageCoords = camera.projectionToImage(realCoordinates);
			minZ = std::min(minZ, imageCoords[2]);
			maxZ = std::max(maxZ, imageCoords[2]);

		}
	}
	if (minZ >= maxZ) {
		qWarning("MinZ and MaxZ invalid. Skip depth map generation.");
		return;
	}
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if(mask[x+ y *width]==0.0f)
				continue;
			float pixelZ = elevation[x + y * width];

			Eigen::Vector3f realCoordinates = pixelToRealCoordinates(x, y, pixelZ);
			Eigen::Vector3f imageCoords = camera.projectionToImage(realCoordinates);
			int pixelValue = (int)round(((imageCoords[2] - minZ) / (maxZ - minZ)) * 255);
			pixelValue = std::min(std::max(pixelValue, 0), 255);

			int imageX = (int)round(imageCoords[0]);
			int imageY = (int)round(imageCoords[1]);

			if (imageX >= 0 && imageX < camera.width && imageY >= 0 && imageY < camera.height) {
				depthMapImage.setPixel(imageX, imageY, qRgb(pixelValue, pixelValue, pixelValue));
				//cout << "Pixel projected (" << x << ", " << y << ") -> (" << imageX << ", " << imageY << "), Z = "
				// << pixelZ << ", pixelValue = " << pixelValue << endl;
			}
		}
	}
	depthMapImage.save(outputPath, "png");
}

void OrthoDepthmap::resizeNormals (int factorPowerOfTwo, int step) {
	int factor = 1 << factorPowerOfTwo;
	Depthmap::resizeNormals(factorPowerOfTwo, step);
	resolution *= factor;
	origin /= factor;
}

void OrthoDepthmap::loadPointCloud(const char *textPath){

	//apri file di testo e scarta tutto quello che non è un numero. fai un char vettore di punti
	QFile file(textPath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		throw QString("Error opening input file: ")+ textPath;
	}

	QTextStream in(&file);


	while (!in.atEnd()) {
		QString line = in.readLine().trimmed();
		QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
		if (line.isEmpty() || (!line[0].isDigit() && line[0] != '-' && line[0] != '+') || parts.size() != 6) {
			continue;
		}

		//check the line if the line not begin with float number break
		bool isValid = true;
		Eigen::Vector3f v;
		for (int i = 0; i < 3; ++i) {
			bool isNumber = false;
			v[i] = parts[i].toFloat(&isNumber);
			if (!isNumber) {
				isValid = false;
				break;
			}
		}
		if (!isValid)
			throw QString("Invalide ply");

		point_cloud.push_back(v);
	}
}

Eigen::Vector3f OrthoDepthmap::realToPixelCoord(float realX, float realY, float realZ){
	float pixelX = (realX - origin[0]) / resolution[0];
	float pixelY = (realY - origin[1]) / resolution[1];
	float h = (realZ - origin[2]) / resolution[2];
	return  Eigen::Vector3f(pixelX, pixelY, h);

}

void OrthoDepthmap::verifyPointCloud(){

	for(const auto& point : point_cloud){

		float realX = point[0];
		float realY = point[1];
		float realZ = point[2];

		Eigen::Vector3f pixelCoord = realToPixelCoord(realX, realY, realZ);

		Eigen::Vector3f realCoord = pixelToRealCoordinates(pixelCoord[0], pixelCoord[1], pixelCoord[2]);


		//	int pixelX = static_cast<int>(round(pixelCoord[0]));
		//	int pixelY = static_cast<int>(round(pixelCoord[1]));

		int pixelX = static_cast<int>(round(pixelCoord[0]));
		int pixelY = static_cast<int>(round(pixelCoord[1]));

		if (pixelX < 0 || pixelX >= width || pixelY < 0 || pixelY >= height) {
			continue;
		}
		/*	cerr << "point inside the image limits "
			 << "Point 3D: (" << realX << ", " << realY << ", " << pixelCoord[2] << "), "
			 << "Coordinate pixel: (" << pixelX << ", " << pixelY <<  " " << elevation[pixelX + pixelY * width]<< ")\n";
*/
	}
}
//#define PRESERVE_INTERIOR

void OrthoDepthmap::beginIntegration(){

	bool use_depthmap = false;
	if(use_depthmap) {
		//1. togliere dalla point tutti i punti che cadono dentro la maschera
		//2. aggiungere un campionamento regolare dentro la maschera
		int count = 0;
		for(size_t i = 0; i < point_cloud.size(); i++) {

			Eigen::Vector3f point = point_cloud[i];
			Eigen::Vector3f pixel = realToPixelCoord(point[0], point[1], point[2]);

			int mx = std::max<float>(0, std::min<float>(width-1, int(pixel[0])));
			int my = std::max<float>(0, std::min<float>(height-1, int(pixel[1])));

			bool inside = mask[mx + my*width] == 0.0f;
			if(inside)
				continue;

			point_cloud[count++] = point;
		}
		point_cloud.resize(count);

		int step = 1;

		for(int y = 0; y < height; y+= step) {
			for(int x = 0; x < width; x += step) {
				bool inside = (mask[x + y*width] != 0.0f);
				if(!inside)
					continue;
				auto point = pixelToRealCoordinates(x, y, elevation[x + y*width]);
				point_cloud.push_back(point);
			}
		}
	}

	for(size_t i =0; i < elevation.size(); i++) {
#ifdef PRESERVE_INTERIOR
		if(mask[i] == 0.0f){
#endif
			elevation[i] = 0.0f;
#ifdef PRESERVE_INTERIOR
		}
#endif
	}
	//foto von bilinear, se non funziona riduci le dimensione x4  con image magik a parte con for
	// guarda quanto è un pixel. scali la depth anche dell rti, con image magik

	//	elevation.clear();
	//	elevation.resize(width * height, 0);
	weights.clear();
	weights.resize(width * height, 0);

}

void OrthoDepthmap::endIntegration(){
	for(size_t i =0; i < elevation.size(); i++){
#ifdef PRESERVE_INTERIOR
		if(mask[i] == 0.0f) {
#endif
			if(weights[i] != 0.0f) {
				elevation[i] /= weights[i];
				mask[i] = 1;
			}
#ifdef PRESERVE_INTERIOR
		}
#endif
	}
}

void OrthoDepthmap::integratedCamera(const CameraDepthmap& camera, const char *outputFile){

	QFile outFile(outputFile);
	if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		cerr << "Errore nell'aprire il file di output: " << outputFile << endl;
		return;
	}
	//test
	float z = point_cloud[0][2];
	auto o = camera.camera.projectionToImage(Eigen::Vector3f(0, 0, z));
	auto u = camera.camera.projectionToImage(Eigen::Vector3f(1, 0, z));

	QTextStream out(&outFile);
	std::vector<Eigen::Vector3f> imageCloud;
	std::vector<float> source;

	for (size_t i = 0; i < point_cloud.size(); i++) {

		Eigen::Vector3f realCoord = point_cloud[i];
		float h = realCoord[2];
		Eigen::Vector3f pixelCoord = realToPixelCoord(realCoord[0], realCoord[1], realCoord[2]);
		// project from ortho plane to camera plane, hence the fixed z
		realCoord[2] = z;
		Eigen::Vector3f imageCoords = camera.camera.projectionToImage(realCoord);
		int pixelX = static_cast<int>(round(imageCoords[0]));
		int pixelY = static_cast<int>(round(imageCoords[1]));


		if (pixelX >= 0 && pixelX < camera.width && pixelY >= 0 && pixelY < camera.height) {
			float depthValue = camera.elevation[pixelX + pixelY * camera.width];
			imageCloud.push_back(Eigen::Vector3f(imageCoords[0]/camera.width, imageCoords[1]/camera.height, h));
			source.push_back(depthValue);
			out << depthValue << "\t" << h << "\t" << imageCoords[0]/camera.width << "\t" << imageCoords[1]/camera.height <<"\n"; //red e green
		}

	}

	GaussianGrid gaussianGrid;
	cout << "minSamples: " << gaussianGrid.minSamples << ", sideFactor: " << gaussianGrid.sideFactor << endl;

	gaussianGrid.minSamples = 1; // 1, 3, 5
	gaussianGrid.sideFactor = 0.25; // 0.25, 0.5, 1, 2, 0.125
	gaussianGrid.init(imageCloud, source);
	cout << "Dopo init: minSamples: " << gaussianGrid.minSamples << ", sideFactor: " << gaussianGrid.sideFactor << endl;

	gaussianGrid.imageGrid(("test.png"));

	float ortho_z = realToPixelCoord(0,0,z)[2];

	for(size_t y=0; y < height; y++){
		for(size_t x=0; x < width; x++){
#ifdef PRESERVE_INTERIOR
			if(mask[x + y * width] != 0.0f){
				continue;
			}
#endif
			Eigen::Vector3f r = pixelToRealCoordinates(x, y, ortho_z);
			assert(fabs(z-r[2]) < 0.1f);
			Eigen::Vector3f p = camera.camera.projectionToImage(r);

			int px = round(p[0]);
			int py = round(p[1]);

			if(px >= 0 && px< camera.width && py >= 0 && py< camera.height){
				float depthValue = camera.elevation[px + py * camera.width];
				p[0] = px / float(camera.width);
				p[1] = py / float(camera.height);

				float h = gaussianGrid.target(p[0], p[1], depthValue);
				Eigen::Vector3f d = realToPixelCoord(r[0], r[1], h);

				float w = camera.calculateWeight(px, py);
				//p0 e p1 devono venire uguale e vedi se depth è ugusle, h dovrebbe venire simile

				elevation[x + y * width] += w * d[2];

				weights[x+ y * width] += w;

			} else {
				//elevation[x + y*width] = origin[2] + resolution[2] * elevation[x + y*width];
			}
		}
	}
}
