#include "depthmap.h"
#include <iostream>
#include <QFile>
#include <QDomDocument>
#include <eigen3/Eigen/Dense>

using namespace std;

int main(int argc, char *argv[]) {
	/*if(argc != 3) {
		cerr << "Usage: " << argv[0] << "<input.tiff> <output.png>" << endl;
		return 1;
	}*/
	QString depthmapPath = "/Users/erika/Desktop/testcenterRel_copia/photogrammetry/Malt/Z_Num7_DeZoom4_STD-MALT.tif";
	QString orientationXmlPath = "/Users/erika/Desktop/testcenterRel_copia/photogrammetry/Ori-Relative/Orientation-L05C12.tif.xml";
	QString output = "out.png";
	Depthmap depth;
	depth.load(qPrintable(depthmapPath));
	depth.computeNormals();
	depth.saveNormals(qPrintable(output));
	depth.saveObj("/Users/erika/Desktop/testcenterRel_copia/photogrammetry/output.obj");

	Camera camera;
	camera.loadXml(orientationXmlPath);
	int pixelX = 165;
	int pixelY = 144;
	float pixelZ = 4.5;


	Eigen::Matrix3f rotationMatrix;
	Eigen::Vector3f center;

	//depth.getOrientationVector(orientationXmlPath, rotationMatrix, center);
	Eigen::Vector3f realCoordinates = depth.pixelToRealCoordinates(pixelX, pixelY, pixelZ);
	realCoordinates[0] =  -0.1216933;
	realCoordinates[1] = 0.7094725;
	realCoordinates[2] = -10.4198828;
	Eigen::Vector3f imageCoords = camera.projectionToImage(realCoordinates);

	cout << "Rotation matrix:"<< endl << camera.rotation << endl;
	cout << "Central vector: (" << camera.center << endl;
	cout << "Real coordinates: (" << realCoordinates[0] << ", "
			  << realCoordinates[1] << ", " << realCoordinates[2] << ")" << endl;

	cout << "Coordinate immagine: (" << imageCoords[0] << ", " << imageCoords[1] << ")" << endl;

	return 0;
}

