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
	//input
//#define MACOS 1
#ifdef MACOS
	QString base = "/Users/erika/Desktop/";
#else
	QString base = "/home/erika/";
#endif

	QString depthmapPath = base + "testcenterRel_copia/photogrammetry/Malt/Z_Num7_DeZoom4_STD-MALT.tif";
	QString orientationXmlPath = base + "testcenterRel_copia/photogrammetry/Ori-Relative/Orientation-L05C12.tif.xml";
	QString maskPath = base + "testcenterRel_copia/photogrammetry/Malt/Masq_STD-MALT_DeZoom4.tif";
	QString plyFile = base +"testcenterRel_copia/photogrammetry/AperiCloud_Relative_mini.ply";
	Depthmap depth;

	//output
	QString outputPath = base + "testcenterRel_copia/photogrammetry/depthmap_projectL05C13.png";
	QString output_mask = base + "testcenterRel_copia/photogrammetry/mask_test.tif";
	QString output_depth = base + "testcenterRel_copia/photogrammetry/depth_test.tif";
	QString output_text = base + "testcenterRel_copia/photogrammetry/point.txt";

	OrthoDepthmap ortho;

	if(!ortho.load(qPrintable(depthmapPath), qPrintable(maskPath))){
		cout << "accidenti" << endl;
		return -1;
	}
	//ortho.computeNormals();
	//ortho.saveNormals(qPrintable(base + "testcenterRel_copia/photogrammetry/original.png"));
	//ortho.saveObj(qPrintable(base + "testcenterRel_copia/photogrammetry/original.obj"));

	ortho.saveDepth(qPrintable(output_depth));
	ortho.saveMask(qPrintable(output_mask));
	ortho.loadText(qPrintable(plyFile), qPrintable(output_text));

	Camera camera;
	camera.loadXml(orientationXmlPath);
	//int pixelX = 165;
	//int pixelY = 144;
	//float pixelZ = 4.5;
	ortho.projectToCameraDepthMap(camera, outputPath);
	Eigen::Matrix3f rotationMatrix;
	Eigen::Vector3f center;

//	depth.loadMask(qPrintable(maskPath));
//	depth.saveDepth(qPrintable(depthmapPath));
//	depth.saveMask(qPrintable(maskPath));
	//QString maskObjPath = base + "testcenterRel_copia/photogrammetry/mask.obj";
//	ortho.saveObj(qPrintable(base + "testcenterRel_copia/photogrammetry/depthmap_projectL05C13.obj"));


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

	cout << "Rotation matrix:"<< endl << camera.rotation << endl;
	cout << "Central vector: (" << camera.center << endl;
	//cout << "Real coordinates: (" << realCoordinates[0] << ", "
	//		  << realCoordinates[1] << ", " << realCoordinates[2] << ")" << endl;

	//cout << "Coordinate immagine: (" << imageCoords[0] << ", " << imageCoords[1] << ")" << endl;


//d = (h + zerolevel) *f scala
	return 0;
}

