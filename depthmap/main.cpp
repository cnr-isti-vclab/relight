#include "depthmap.h"
#include "../src/bni_normal_integration.h"
#include <iostream>
#include <QFile>
#include <QDomDocument>
#include <eigen3/Eigen/Dense>

using namespace std;
// carica il tif, calcola e integra normali per vedere se la superficie è la stessa.
//
int main(int argc, char *argv[]) {
	/*if(argc != 3) {
		cerr << "Usage: " << argv[0] << "<input.tiff> <output.png>" << endl;
		return 1;
	}*/

	QString depthmapPath = "/Users/erika/Desktop/testcenterRel_copia/photogrammetry/Malt/Z_Num7_DeZoom4_STD-MALT.tif";
	QString orientationXmlPath = "/Users/erika/Desktop/testcenterRel_copia/photogrammetry/Ori-Relative/Orientation-L05C12.tif.xml";
	QString maskPath = "/Users/erika/Desktop/testcenterRel_copia/photogrammetry/Malt/Masq_STD-MALT_DeZoom4.tif";
	Depthmap depth;
	depth.load(qPrintable(depthmapPath));
	depth.computeNormals();
	depth.loadMask(qPrintable(maskPath));

	depth.saveNormals("/Users/erika/Desktop/testcenterRel_copia/photogrammetry/original.obj");
	depth.saveObj("/Users/erika/Desktop/testcenterRel_copia/photogrammetry/original.obj");

	depth.depthIntegrateNormals();
	depth.saveObj("/Users/erika/Desktop/testcenterRel_copia/photogrammetry/integrated.obj");

	/*int factorPowerOfTwo = 1;
	depth.resizeNormals(factorPowerOfTwo, 2);
	depth.depthIntegrateNormals();
	depth.saveNormals("/Users/erika/Desktop/testcenterRel_copia/photogrammetry/resized_integrated.jpg");
	depth.saveObj("/Users/erika/Desktop/testcenterRel_copia/photogrammetry/resized_integrated.obj");*/


	QString outputPath = "/Users/erika/Desktop/testcenterRel_copia/photogrammetry/depthmap_project.png";

	Camera camera;
	camera.loadXml(orientationXmlPath);
	//int pixelX = 165;
	//int pixelY = 144;
	//float pixelZ = 4.5;
	depth.projectToCameraDepthMap(camera, outputPath);

	Eigen::Matrix3f rotationMatrix;
	Eigen::Vector3f center;

	//depth.getOrientationVector(orientationXmlPath, rotationMatrix, center);
	//Eigen::Vector3f realCoordinates = depth.pixelToRealCoordinates(pixelX, pixelY, pixelZ);
	//realCoordinates[0] =  -0.1216933;
	//realCoordinates[1] = 0.7094725;
	//realCoordinates[2] = -10.4198828;
	//Eigen::Vector3f imageCoords = camera.projectionToImage(realCoordinates);

	cout << "Rotation matrix:"<< endl << camera.rotation << endl;
	cout << "Central vector: (" << camera.center << endl;
	//cout << "Real coordinates: (" << realCoordinates[0] << ", "
	//		  << realCoordinates[1] << ", " << realCoordinates[2] << ")" << endl;

	//cout << "Coordinate immagine: (" << imageCoords[0] << ", " << imageCoords[1] << ")" << endl;


//d = (h + zerolevel) *f scala
	return 0;
}

