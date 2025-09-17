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
#include "../external/assm/Grid.h"

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

	QFile file(textPath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		throw QString("Error opening input file: ")+ textPath;
	}

	QTextStream in(&file);


	while (!in.atEnd()) {
		QString line = in.readLine().trimmed();
		QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
		//		QStringList parts = line.split(QRegularExpression("\\s+"), QString::SkipEmptyParts);
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


//#define PRESERVE_INTERIOR

void OrthoDepthmap::beginIntegration(){

	int cx = 1025;
	int cy = 776;
	int holeW = 50;
	int holeH = 50;

	int x1 = cx - holeW / 2;
	int y1 = cy - holeH / 2;
	int x2 = cx + holeW / 2;
	int y2 = cy + holeH / 2;


	for (int yy = y1; yy < y2; yy++) {
		for (int xx = x1; xx < x2; xx++) {
			mask[xx + yy * width] = 0.0f;
		}
	}

	// opzionale: salva la mask bucata su file per debug
	Depthmap::saveTiff("mask_with_hole_50x50.tif", mask, width, height, 1);

	bool use_depthmap = false;
	if(use_depthmap) {
		//1. togliere dalla point tutti i punti che cadono dentro la maschera
		//2. aggiungere un campionamento regolare dentro la maschera
		int count = 0;
		float sum_diff = 0.0f;
		float sum_sq_diff = 0.0f;
		float min_diff = 1e9f;
		float max_diff = -1e9f;
		int valid_count = 0;

		for(size_t i = 0; i < point_cloud.size(); i++) {

			Eigen::Vector3f point = point_cloud[i];
			Eigen::Vector3f pixel = realToPixelCoord(point[0], point[1], point[2]);

			int mx = std::max<float>(0, std::min<float>(width-1, int(pixel[0])));
			int my = std::max<float>(0, std::min<float>(height-1, int(pixel[1])));

			bool inside = mask[mx + my*width] == 1.0f;
			if(inside){
				float z = elevation[mx + my * width];
				float diff = fabsf(pixel[2] - z);


				sum_diff += diff;
				sum_sq_diff += diff * diff;
				min_diff = std::min(min_diff, diff);
				max_diff = std::max(max_diff, diff);
				valid_count++;
				//continue;
			}else {
				//	point_cloud[count++] = point;
			}
		}
		point_cloud.resize(count);

		if (valid_count > 0) {
			float mean_diff = sum_diff / valid_count;
			float variance = (sum_sq_diff / valid_count) - (mean_diff * mean_diff);
			float stddev = sqrtf(std::max(0.0f, variance));
			float mse = sum_sq_diff / valid_count;
			float rmse = sqrtf(mse);

			std::cout << "[STATS] Differenza pixel[2] - z (solo inside mask)\n";
			std::cout << "        Count     = " << valid_count << "\n";
			std::cout << "        Min diff  = " << min_diff << "\n";
			std::cout << "        Max diff  = " << max_diff << "\n";
			std::cout << "        Mean diff = " << mean_diff << "\n";
			std::cout << "        Stddev    = " << stddev << "\n";
			std::cout << "        RMSE      = " << rmse << "\n";
		}
		int step = 10;

		/*int x1 = 720;
		int x2 = 1111;
		int y1 = 20;
		int y2 = 504;

		int y1 = 659;
		int y2 = 1577;
		int x1 = 883;
		int x2 = 2145;*/
		/*	int y1 = 504;
		int y2 = 1111;
		int x1 = 883;
		int x2 = 1577;*/


		/*	std::vector<float> elevation_section(width * height, 0.0f);

		for (int y = y1; y <= y2; y += 1) {
			for (int x = x1; x <= x2; x += 1) {

				bool inside = (mask[x + y * width] == 1.0f);
				if (!inside) continue;

				elevation_section[x + y * width] = elevation[x + y * width];

				if(((x % step) == 0) && ((y % step) == 0)) {
					auto point = pixelToRealCoordinates(x, y, elevation[x + y * width]);
					point_cloud.push_back(point);
				}
			}
		}

		Depthmap::saveTiff("quadrante.tif", elevation_section, width, height, 32);
		std::ofstream csv("points.csv");
		if (csv.is_open()) {
			csv << "X,Y,Z\n";
			for (auto &p : point_cloud) {
				csv << p[0] << "," << p[1] << "," << p[2] << "\n";
			}
		}

			for(int y = 0; y < height; y+= step) {
			for(int x = 0; x < width; x += step) {
				bool inside = (mask[x + y*width] == 1.0f);
				if(!inside)
					continue;
				auto point = pixelToRealCoordinates(x, y, elevation[x + y*width]);
				point_cloud.push_back(point);
			}
		}
	}*/
		/*struct Quadrant {
			int x1, y1; // top-left
			int x2, y2; // bottom-right
			std::string name;
		};

		std::vector<Quadrant> quadrants = {
			{720, 20, 1111, 504,  "quad1"},
			{1111, 504, 1577, 883, "quad2"},
			{1577, 659, 2145, 883, "quad3"}  // corretto quad3 con altezza "vera"
		};

		for (const auto& q : quadrants) {
			int cropW = q.x2 - q.x1 + 1;
			int cropH = q.y2 - q.y1 + 1;
			std::vector<float> elevation_crop(width * height, 0.0f);

			for (int y = 0; y < cropH; y++) {
				for (int x = 0; x < cropW; x++) {
					int srcX = q.x1 + x;
					int srcY = q.y1 + y;
					if (srcX < 0 || srcX >= width || srcY < 0 || srcY >= height) continue;

					bool inside = (mask[srcX + srcY * width] == 1.0f);
					if (!inside) continue;

					elevation_crop[srcX + srcY * width] = elevation[srcX + srcY * width];
				}
			}

			std::string filename = q.name + "_crop.tif";
			Depthmap::saveTiff(filename.c_str(), elevation_crop, width, height, 32);
			cout << "Salvato quadrante in " << filename
				 << " (dim=" << cropW << "x" << cropH << ")" << endl;
		}
	}*/


	}


	//exit(0);
	old_elevation = elevation;

	for(size_t i =0; i < elevation.size(); i++) {

		elevation[i] = 0.0f;
	}
	/*std::vector<float> mask_copy = mask;

	std::vector<Eigen::Vector2i> locations = {
		{1025, 776},
		//{1444, 847},
		//{1413, 1010},
		//{1081, 1018}
	};

	int holeW = 200;
	int holeH = 200;

	for (const auto& loc : locations) {
		int cx = loc.x();
		int cy = loc.y();

		int x1 = cx - holeW / 2;
		int y1 = cy - holeH / 2;
		int x2 = cx + holeW / 2;
		int y2 = cy + holeH / 2;

		for (int yy = y1; yy <= y2; yy++) {
			for (int xx = x1; xx <= x2; xx++) {
				mask_copy[xx + yy * width] = 0.0f;
			}
		}
	}

	Depthmap::saveTiff("mask_region_200x200.tif", mask_copy, width, height, 1);
*/
	/*
	int holeW = 50;
	int holeH = 50;

	int x1 = 800; // esempio top-left x
	int y1 = 1100;  // esempio top-left y
	int x2 = x1 + holeW;
	int y2 = y1 + holeH;


	// applico il buco (metto mask = 0 nella regione 50x50)
	for (int yy = y1; yy < y2; yy++) {
		for (int xx = x1; xx < x2; xx++) {
			mask[xx + yy * width] = 0.0f;
		}
	}

	*/
	//foto von bilinear, se non funziona riduci le dimensione x4  con image magik a parte con for
	// guarda quanto è un pixel. scali la depth anche dell rti, con image magik

	//elevation.clear();
	//elevation.resize(width * height, 0);
	weights.clear();
	weights.resize(width * height, 0);


	/*for (int i = 0; i < width * height; ++i) {
		if (mask[i] != 0.0f) {
			weights[i] = 1.0f;
		}
	}*/

}


//  e un blending sulla maschera e salvare una copia dell'elevation
// in modo tale che la depth Micmac(?) prenda la depth dell rti quando è 0.5 e quando è 0 prenda la depth del micmac così da riempire i punti.
void OrthoDepthmap::endIntegration(){

	for(size_t i =0; i < elevation.size(); i++){
#ifdef PRESERVE_INTERIOR
		if(mask[i] == 0.0f) {
#endif
			if(weights[i] != 0.0f) {
				//	elevation[i] /= weights[i];
			}

#ifdef PRESERVE_INTERIOR
		}
#endif
	}

	int mask_zeros = 0;
	int mask_ones = 0;
	float min_mask = 1e9f;
	float max_mask = -1e9f;

	for (float v : mask) {
		if (v == 0.0f) mask_zeros++;
		if (v == 1.0f) mask_ones++;
		min_mask = std::min(min_mask, v);
		max_mask = std::max(max_mask, v);
	}

	cout << "[Check] mask contiene:\n";
	cout << "         " << mask_zeros << " valori = 0.0\n";
	cout << "         " << mask_ones << " valori = 1.0\n";
	cout << "[DEBUG] mask range: min = " << min_mask << ", max = " << max_mask << endl;



	Grid<float> blurred(width, height, 0.0f);
	for (int y = 0; y < int(height); y++)
		for (int x = 0; x < int(width); x++){
			blurred.at(y, x) = mask[x + y * width];
		}


	// set the value for blur
	float blur_radius_pixel = 50.0f;
	int kernelSize = int(8 * std::ceil(blur_radius_pixel) + 1);
	if (kernelSize % 2 == 0) kernelSize += 1;

	//valori di mask

	int center_x = width / 2;
	int center_y = height / 2;
	int center_index = center_y * width + center_x;

	float center_mask_value = blurred[center_index];

	if (center_mask_value == 1.0f) {
		cout << "[Check] Il centro (x=" << center_x << ", y=" << center_y << ") dell'input al blur è 1.0" << endl;
	} else {
		cerr << "[Warning] Il centro dell'input al blur NON è 1.0, ma " << center_mask_value << endl;
	}


	float center_mask_raw = mask[center_index];
	cout << "[Check] Il centro (x=" << center_x << ", y=" << center_y << ") della mask ha valore: "
		 << center_mask_raw << endl;


	//valori di  blur)
	int input_zeros = 0;
	int input_ones = 0;
	for (int i = 0; i < width * height; ++i) {
		if (blurred[i] == 0.0f) input_zeros++;
		else if (blurred[i] == 1.0f) input_ones++;
	}

	cout << "[Check] Input al blur contiene:\n";
	cout << "         " << input_zeros << " valori = 0.0\n";



	blurred = blurred.gaussianBlur(kernelSize, blur_radius_pixel);



	// Rimappa il range [0.5, 1] to [0, 1]
	blurred_mask.resize(width * height);
	for (int i = 0; i < width * height; i++) {
		float v = blurred[i];
		if (v <= 0.5f) {
			blurred_mask[i] = 0.0f;
		} else {
			blurred_mask[i] = (v - 0.5f) * 2.0f;
		}
	}
	// Debug: range dei valori nel blurred prima del remapping
	float min_blurred = 1e9f;
	float max_blurred = -1e9f;

	for (float v : blurred) {
		min_blurred = std::min(min_blurred, v);
		max_blurred = std::max(max_blurred, v);
	}

	cout << "[DEBUG] blurred range: min = " << min_blurred << ", max = " << max_blurred << endl;

	// Rimappa il range [0.5, 1.0] → [0, 1] solo per valori > 0.5
	blurred_mask.resize(width * height);
	int count_blurmask_zeros = 0;
	int count_blurmask_ones = 0;

	for (int i = 0; i < width * height; i++) {
		float v = blurred[i];
		if (v <= 0.5f) {
			blurred_mask[i] = 0.0f;
			count_blurmask_zeros++;
		} else if (v >= 1.0f) {
			blurred_mask[i] = 1.0f;
			count_blurmask_ones++;
		} else {
			blurred_mask[i] = (v - 0.5f) * 2.0f;
			if (blurred_mask[i] == 1.0f)
				count_blurmask_ones++;
		}
	}

	cout << "[Check] blurred_mask contiene:\n";
	cout << "         " << count_blurmask_zeros << " valori = 0.0\n";
	cout << "         " << count_blurmask_ones << " valori = 1.0\n";


	/*blurred_mask.resize(width * height);
	float minv = 1e9f, maxv = -1e9f;
	for (float v : blurred)
		minv = std::min(minv, v), maxv = std::max(maxv, v);


	for (int i = 0; i < width * height; i++) {
		float v = blurred[i];
		blurred_mask[i] = (v <= 0.5f) ? 0.0f : (v - 0.5f) * 2.0f;
	}*/


	// Blending tra elevation e old_elevation
	for (size_t i = 0; i < elevation.size(); i++) {
		float blur_weight = blurred_mask[i]; // 0 = MicMac, 1 = RTI
		if(weights[i] > 0.0f){
			mask[i] = 1.0f;
			elevation[i] /= weights[i];
		}
		elevation[i] = blur_weight * old_elevation[i] +
					   (1.0f - blur_weight) * elevation[i];


	}
}
// vogliamo prendere la parte tra 0.5 e 1 e convertirla tra 0 e 1
// devo sottrarre 0.5 e moltiplicare per 2
// ------------------------------------------------------

void OrthoDepthmap::integratedCamera(const CameraDepthmap& camera, const char *outputFile){

	QFile outFile(outputFile);
	if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		cerr << "Errore nell'aprire il file di output: " << outputFile << endl;
		return;
	}
	//test
	float z = point_cloud[0][2];
	//auto o = camera.camera.projectionToImage(Eigen::Vector3f(0, 0, z));
	//auto u = camera.camera.projectionToImage(Eigen::Vector3f(1, 0, z));

	QTextStream out(&outFile);
	std::vector<Eigen::Vector3f> imageCloud;
	std::vector<float> source;

	out << "x\ty\tz\n";
	for (size_t i = 0; i < point_cloud.size(); i++) {

		Eigen::Vector3f realCoord = point_cloud[i];
		float h = realCoord[2];
		Eigen::Vector3f pixelCoord = realToPixelCoord(realCoord[0], realCoord[1], realCoord[2]);
		// project from ortho plane to camera plane, hence the fixed z
		//realCoord[2] = z;
		Eigen::Vector3f imageCoords = camera.camera.projectionToImage(realCoord);
		int pixelX = static_cast<int>(round(imageCoords[0]));
		int pixelY = static_cast<int>(round(imageCoords[1]));


		if (pixelX >= 0 && pixelX < camera.width && pixelY >= 0 && pixelY < camera.height) {
			float depthValue = camera.elevation[pixelX + pixelY * camera.width];
			imageCloud.push_back(Eigen::Vector3f(imageCoords[0]/camera.width, imageCoords[1]/camera.height, imageCoords[2]));
			source.push_back(depthValue);
			//test
			/*if (pixelCoord[0] >= 0 && pixelCoord[1] >= 0 && pixelCoord[0] < width && pixelCoord[1] < height) {

				imageCloud.push_back(Eigen::Vector3f(imageCoords[0]/camera.width, imageCoords[1]/camera.height, pixelCoord[2]));
				source.push_back(old_elevation[int(pixelCoord[0]) + int(pixelCoord[1])*width]);
			}*/
		}
	}



	GaussianGrid gaussianGrid;
	cout << "minSamples: " << gaussianGrid.minSamples << ", sideFactor: " << gaussianGrid.sideFactor << endl;

	gaussianGrid.minSamples = 1;//TODO: 3 seems a good value // 1, 3, 5
	gaussianGrid.sideFactor = 1; //0.25; //0.125; // 0.25, 0.5, 1, 2, 0.125
	gaussianGrid.init(imageCloud, source);
	cout << "Dopo init: minSamples: " << gaussianGrid.minSamples << ", sideFactor: " << gaussianGrid.sideFactor << endl;

	gaussianGrid.imageGrid(("test3_0125.png"));

	Depthmap::saveTiff("grid.tiff", gaussianGrid.values, gaussianGrid.width, gaussianGrid.height, 32);




	//proietta il depth map del rti sull ortho
	//1. crea array grande quantp l ortho in float inizializzalo a 0
	std::vector<float> orthoDepth(width * height, 0.0f);
	//2. itera su x e y dell img che crei width e height di depth ortho, cord x e y ci mettiamola z e proietta in cord reali da ortho a real
	//float z = point_cloud[0][2];
	/*Eigen::Vector3f realTest(-1.268, 1.345, -8.462);
	Eigen::Vector3f pixelTest = camera.camera.projectionToImage(realTest);
	Eigen::Vector3f back = camera.camera.projectionToReal(pixelTest);

	cout <<  "Real: " << realTest << endl;
	cout << "Coords: " << pixelTest << endl;
	cout << "Back-projected: " << back << endl;*/


	//3. variabile z
	//proietta nella camera cameraPr, controlliamo che stia dentro, se sta dentro prendiamo l'elevation e si scrive nell immagine.
	for (int y = 0; y < camera.height; y++) {
		for (int x = 0; x < camera.width; x++) {
			Eigen::Vector2f p;
			p[0] = x / float(camera.width);
			p[1] = y / float(camera.height);

			float depthValue = camera.elevation[x + y * camera.width];
			float z = gaussianGrid.target(p[0], p[1], depthValue);

			Eigen::Vector3f realCoord = camera.camera.projectionToReal(Eigen::Vector3f(x, y, z));
			Eigen::Vector3f pixelCoords = realToPixelCoord(realCoord[0], realCoord[1], realCoord[2]);

			int ox = int(pixelCoords[0]);
			int oy = int(pixelCoords[1]);
			if(ox< 0 || ox >=width || oy < 0 || oy >= height)
				continue;
			orthoDepth[ox + oy * width] = pixelCoords[2];
			float w = camera.calculateWeight(x, y);
			//p0 e p1 devono venire uguale e vedi se depth è ugusle, h dovrebbe venire simile
			elevation[ox + oy * width] += w * pixelCoords[2];

			weights[ox+ oy * width] += w;
		}
	}
	{
		std::ofstream csv("integrated_points.csv");
		if (csv.is_open()) {
			csv << "X,Y,Z,Elevation,Diff\n";
			for (size_t i = 0; i < imageCloud.size(); i++) {
				const auto& p = imageCloud[i];
				float elev = source[i];
				float interpolate_h = gaussianGrid.target(p[0], p[1], elev);
				csv << p[0] << "," << p[1] << "," << p[2] << "," << elev << "," << interpolate_h << "\n";

			}
		}
	}


	Depthmap::saveTiff("ortho_projection.tif", orthoDepth, width, height, 32);

}
void OrthoDepthmap::saveBlurredMask(const char* filename) const {
	QImage img(width, height, QImage::Format_Grayscale8);

	float minVal = 1.0f, maxVal = 0.0f;
	for (float v : blurred_mask) {
		if (v < minVal) minVal = v;
		if (v > maxVal) maxVal = v;
	}

	float range = std::max(1e-5f, maxVal - minVal);

	for (int y = 0; y < int(height); y++) {
		for (int x = 0; x < int(width); x++) {
			float value = blurred_mask[x + y * width];
			value = (value - minVal) / range;
			value = std::max(0.0f, std::min(1.0f, value));
			int gray = int(value * 255.0f);
			img.setPixel(x, y, qRgb(gray, gray, gray));
		}
	}
	img.save(filename);
}
