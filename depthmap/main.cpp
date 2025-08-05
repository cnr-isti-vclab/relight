#include "depthmap.h"
#include "../src/bni_normal_integration.h"
#include <iostream>
#include <QFile>
#include <QDir>
#include <QFileInfoList>
#include <QDomDocument>
#include <eigen3/Eigen/Dense>
#include <math.h>
#include "camera.h"
#include "gaussiangrid.h"
#include "orthodepthmap.h"

using namespace std;
// carica il tif, calcola e integra normali per vedere se la superficie è la stessa.
//
int main(int argc, char *argv[]) {
	/*if(argc != 3) {
		cerr << "Usage: " << argv[0] << "<input.tiff> <output.png>" << endl;
		return 1;
	}*/
	//input
#define MACOS 1
#ifdef MACOS
	QString base = "/Users/erika/Desktop/testProva/";
#else
	QString base = "";
#endif



	QString depthmapPath = base + "photogrammetry/Malt/Z_Num7_DeZoom4_STD-MALT copia.tif";
	//QString cameraDepthmap = base + "datasets/L04C12.tif";
	//QString orientationXmlPath = base + "photogrammetry/Ori-Relative/Orientation-L04C12.tif.xml";
	QString maskPath = base + "photogrammetry/Malt/Masq_STD-MALT_DeZoom4 copia.tif";
	QString plyFile = base + "photogrammetry/AperiCloud_Relative__mini.ply";
	//QString point_txt = base + "photogrammetry/points_h.txt";
	Depthmap depth;

	//output
	QString outputPath = base + "tdepthmap_projectL05C13.png";
	QString output_mask = base + "mask_test.tif";
	QString output_depth = base + "depth_test.tif";
	QString output_points = base + "points_h.txt";
	QString output_grid = base + "tgrid.png";


	OrthoDepthmap ortho;


	/*QFile::remove(depthmapPath);
	if (!QFile::copy(depthmapPath + "_backup.tif", depthmapPath)) {
		cout << "Error copying depthmap " << depthmapPath.toStdString() << endl;
		exit(0);
	}
	QFile::remove(maskPath);
	if (!QFile::copy( maskPath + "_backup.tif", maskPath)) {
		cout << "Error copying mask" << endl;
		exit(0);
	}
*/
	if(!ortho.load(qPrintable(depthmapPath), qPrintable(maskPath))){
		cout << "accidenti" << endl;
		return -1;
	}

	QDir datasetsDir(base + "datasets");
	QDir xmlDir(base + "photogrammetry/Ori-Relative");

	QStringList extensions = {".tiff", ".tif", ".jpg", ".jpeg"};
	QStringList tiffFilters = {"*.tiff"};


	QFileInfoList tiffFiles = datasetsDir.entryInfoList(tiffFilters, QDir::Files);
	if (tiffFiles.isEmpty()) {
		cerr << "No .tiff files found in " << datasetsDir.absolutePath().toStdString() << endl;
		return -1;
	}

	QFileInfoList xmlFiles = xmlDir.entryInfoList({"*.xml"}, QDir::Files);
	if (xmlFiles.isEmpty()) {
		cerr << "No .xml files found in " << xmlDir.absolutePath().toStdString() << endl;
		return -1;
	}


//doortho = 1 domec =0;

	//ortho.computeNormals();
	//ortho.saveNormals(qPrintable(base + "testcenterRel_copia/photogrammetry/original.png"));
	//ortho.saveObj(qPrintable(base + "testcenterRel_copia/photogrammetry/original.obj"));
	try{
		ortho.loadDepth(qPrintable(depthmapPath));
		ortho.loadMask(qPrintable(maskPath));
		ortho.loadPointCloud(qPrintable(plyFile));
	} catch(QString e){
		cout << qPrintable(e) << endl;
		exit(-1);
	}
	//ortho.saveMask(qPrintable(output_mask));

	ortho.verifyPointCloud();
	ortho.beginIntegration();


	for (const QFileInfo &tiffFile : tiffFiles) {

		CameraDepthmap depthCam;
		QString cameraName = tiffFile.completeBaseName();
		QString orientationXmlPath;
		for (const QString &ext : extensions) {
			QString potentialPath = xmlDir.absoluteFilePath("Orientation-" + cameraName + ext + ".xml");
			if (QFile::exists(potentialPath)) {
				orientationXmlPath = potentialPath;
				break;
			}
		}
		/*QString orientationXmlPath = xmlDir.absoluteFilePath("Orientation-" + cameraName + ext + ".xml");

		cout << "Looking for XML: " << orientationXmlPath.toStdString() << endl;*/

		if (!depthCam.camera.loadXml(orientationXmlPath)) {
			cerr << "Failed to load XML: " << orientationXmlPath.toStdString() << endl;
			continue;
		}

		if (!depthCam.loadDepth(qPrintable(tiffFile.absoluteFilePath()))) {
			cerr << "Failed to load depth map: " << tiffFile.fileName().toStdString() << endl;
			return -1;
		}
		if(depthCam.width != depthCam.camera.width || depthCam.height != depthCam.camera.height){
			cerr << "width is not the same" << endl;
			return -1;
		}
		cout << "Processed: " << tiffFile.fileName().toStdString() << endl;
		ortho.integratedCamera(depthCam, qPrintable(output_points));
		//ortho.projectToCameraDepthMap(depthCam.camera, outputPath);

		QString outputTiffPath = base +"output_" + tiffFile.fileName();
		cout << "Output TIFF Path: " << outputTiffPath.toStdString() << endl;

	}
	ortho.endIntegration();
	ortho.saveDepth(qPrintable(output_depth));
	ortho.saveMask(qPrintable(output_mask));
	ortho.saveObj("weightsElev3_0125.obj");
	//ortho.saveBlurredMask(qPrintable(base + "mask_blurred.tif"));
	//ortho.saveBlurredMask(qPrintable(base + "blurred_mask.tif"));





		//depthCam.camera.loadXml(orientationXmlPath);
		//depthCam.loadDepth(qPrintable(tiffFile.absoluteFilePath()));

	//ortho.saveDepth(qPrintable(base + "testDepth.tiff"));
	//ortho.computeGaussianWeightedGrid(qPrintable(point_txt));


	//int pixelX = 165;
	//int pixelY = 144;
	//float pixelZ = 4.5;

	Eigen::Matrix3f rotationMatrix;
	Eigen::Vector3f center;

	//	depth.loadMask(qPrintable(maskPath));
	//	depth.saveDepth(qPrintable(depthmapPath));
	//	depth.saveMask(qPrintable(maskPath));
	//QString maskObjPath = base + "testcenterRel_copia/photogrammetry/mask.obj";
	//ortho.saveObj(qPrintable(base + "depthmap_projectL05C13.obj"));


	//depth.depthIntegrateNormals();
	//	ortho.saveObj(qPrintable(base + "testcenterRel_copia/photogrammetry/integrated.obj"));




	// modifica maschere e depth map per migliorare la depth

	/*1. test prendi varie masq e modificale in maniera consistente: 1. masq depth 2. masq per ogni camera, cosa modifico?
	tawny usa la masq della depth da rti a ortho piano, quelle per camera le usa quando fa l'orthopiano mosaico
carica ortho, serve un write per scriverla, prima di scriverla la riempiamo. Quello che scrive deve riscrivere anche la masq.
2 funzioni per scrivere i tiff. riempi la masq di 1. All'inizio si mette un valore medio, poi si fitta la map rti farla tornare con
la depth map orthopiano, dopo si lancia il tawny.
Prendi un punto ogni 100 pixel della depth. che è nell stess spazio rti: A rti -> A depth Br -> Bd, prendi il dz. scalare nello stesso
spazio prendendo la media. ALLINEAMENTO GROSSOLANO DELLA SUPERFICIE, CENTRARE L'ORIGINE E LA SCALA. sappiamo che sono allineati i pixel
quanto larga farla deve tornare con l'immagine. scala quanto larga deve essere, origine quanto su o giù deve essere. l'1 può essere assunto coem la scala
devi trovare la z facendo la media (tra spazio camera e spazio reale). per ogni punto di rti abbiamo una dz del mask. dalla masq all rti.
per ogni punto c è un dz ci sono freq lunghe e freq corte, per il dz portare le freq lunghe della depth e freq corte del rti (tranne i buchi).
I buchi: Laplaciano è un operatore cjhe trasforma un campo di valori come nel gas e il calore si espande. prendo un punto e faccio la media dei 4 valori intorno ecc.
fino a che non cambia più. per frequenze basse lo fccio su una griglia campionata, fai l'interpolazione bilineare. dz la apllico a rti

3b. corrispondenze salvate nel xml


interpolazione rti e masq calcoli i puntiprendi la media nelle zone dove non hai il fit perchè c è un buco. Inverso della proiezione:
fai la formula inversa, inverti la matrice. interpola bilinarmente dati 4 valori trova un punto nel mezzo in una superficie lineare.

1. poter scrivere la depth map e la masq, fai save con i nomi
2. prendi la superficie dell rti e trovi la media delle due superfici della depth e della rti. sottraiamo la media del rti per vedere se hanno la stessa media.
3. fai il dz, 1 un punto ogni 100 pixel
4. laplaciano: ogni pixel non conosciuto prende la media dei pixel vicini conosciuti fa la media dfinche non si ferma
5. interpolazione bilineare per poterla applicare a Rti, per togliere i buchi e riempire i dettagli.
6. riproietta la superficie sull'orthopiano
7. blending di tutte le superfici che arrivano dagli rti. quello da definire.

1a. Depthmap devi salvarlo in obj per vederla in conftonto con rti.
3a. devi salvarla come img per vedere cosa viene (trova min e max e scala * 255).
6a. salva come img.




1b. prendi i punti in Apericloud (lo lanci per il formato testo), prendiamo i punti e li proiettiamo,(stessa per la proiezione delle coord 3d)
prendi un punto se va fuori si butta, sennò vediamo in che punto della griglia cade, per ogni blocco griglia cerchiamo di capire
la funzione, h di partenza e h di arrivo quindi abbiamo la coppia (la z si ricostruisce dall'altro lato), quando non si ha niente nei quadratini
serve interpolare; quando c è solo una coppia non si può stimare niente; con più coppie si stiamano la A e la B. (devi fare la regressione
lineare per la serie di coppie per trovare a e b). Pesa i punti.
2b. valori calcolati e non, per quelli non si fa la diffusione laplaciana, peso che va per distanza, stessa riga e stessa colonna.
3b. prendi tutti i pixel

 */
	/*int factorPowerOfTwo = 1;
	depth.resizeNormals(factorPowerOfTwo, 2);
	depth.depthIntegrateNormals();
	depth.saveNormals(base + "testcenterRel_copia/photogrammetry/resized_integrated2.jpg");
	depth.saveObj(base + "testcenterRel_copia/photogrammetry/resized_integrated2.obj");*/






	//depth.getOrientationVector(orientationXmlPath, rotationMatrix, center);
	//Eigen::Vector3f realCoordinates = depth.pixelToRealCoordinates(pixelX, pixelY, pixelZ);
	//realCoordinates[0] =  -0.1216933;
	//realCoordinates[1] = 0.7094725;
	//realCoordinates[2] = -10.4198828;
	//Eigen::Vector3f imageCoords = camera.projectionToImage(realCoordinates);

	//cout << "Rotation matrix:"<< endl << camera.rotation << endl;
	//cout << "Central vector: (" << camera.center << endl;
	//cout << "Real coordinates: (" << realCoordinates[0] << ", "
	//		  << realCoordinates[1] << ", " << realCoordinates[2] << ")" << endl;

	//cout << "Coordinate immagine: (" << imageCoords[0] << ", " << imageCoords[1] << ")" << endl;


	//d = (h + zerolevel) *f scala
	return 0;
}

