#include <tiffio.h>
#include <iostream>
#include <QImage>
#include <QDir>
#include <QDomElement>
#include <QtXml/QDomDocument>
#include "depthmap.h"
#include "../src/bni_normal_integration.h"
#include <QFile>

using namespace std;


bool Depthmap::loadTiff(const char *tiff, vector<float> &values, uint32_t &w, uint32_t &h) {
	TIFF* inTiff = TIFFOpen(tiff, "r");

	if (!inTiff) {
		cerr << "Could not open input TIFF file." << endl;
		return false;
	}

	// Get image width, height, and other necessary tags
	TIFFGetField(inTiff, TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField(inTiff, TIFFTAG_IMAGELENGTH, &h);

	uint16_t samplesPerPixel = 1;
	TIFFGetField(inTiff, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
	if(samplesPerPixel != 1) {
		cerr << "Not a depthmap, expecting just 1 channel" << endl;
		TIFFClose(inTiff);
		return false;
	}

	uint16_t bitsPerSample = 32;
	TIFFGetField(inTiff, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
	if(bitsPerSample != 32 && bitsPerSample !=1) {
		cerr << "Samples should be a float 32 bit or 1 bit boolean" << endl;
		TIFFClose(inTiff);
		return false;
	}

	uint16_t sampleFormat = SAMPLEFORMAT_IEEEFP; // Floating-point data
	//TIFFGetField(inTiff, TIFFTAG_SAMPLEFORMAT, &sampleFormat);
	if (!TIFFGetField(inTiff, TIFFTAG_SAMPLEFORMAT, &sampleFormat)) {
		cerr << "Failed to retrieve SAMPLEFORMAT tag." << endl;
		TIFFClose(inTiff);
		return false;
	}

	values.resize(w * h);

	// Check if the TIFF is tiled
	uint32_t tileWidth, tileLength;
	if (!TIFFGetField(inTiff, TIFFTAG_TILEWIDTH, &tileWidth) ||
		!TIFFGetField(inTiff, TIFFTAG_TILELENGTH, &tileLength)) {
		return loadStripedTiff(inTiff, values, w, h, bitsPerSample);
	} else {
		return loadTiledTiff(inTiff, values, w, h, tileWidth, tileLength, bitsPerSample);
	}
	tsize_t tileSize = TIFFTileSize(inTiff);
	if (tileSize == 0) {
		cerr << "Error computing tile size." << endl;
		TIFFClose(inTiff);
		return 1;
	}

	return true;

}
//TODO: controllare che sia effettivamenet floating point 32 bit


bool Depthmap::loadTiledTiff(TIFF* inTiff, vector<float> &values, uint32_t w, uint32_t h,
							 uint32_t tileWidth, uint32_t tileLength, uint32_t bitsPerSample){

	tsize_t tileSize = TIFFTileSize(inTiff);
	if (tileSize == 0) {
		cerr << "Error computing tile size." << endl;
		TIFFClose(inTiff);
		return false;
	}
	// Compute tile size and total number of tiles

	uint32_t numTilesX = (w + tileWidth - 1) / tileWidth;
	uint32_t numTilesY = (h + tileLength - 1) / tileLength;


	if(bitsPerSample==32){
		vector<float> tileData(tileSize / sizeof(float));

		for (uint32_t y = 0; y < numTilesY; ++y) {
			for (uint32_t x = 0; x < numTilesX; ++x) {
				uint32_t tileIndex = TIFFComputeTile(inTiff, x * tileWidth, y * tileLength, 0, 0);

				if (TIFFReadEncodedTile(inTiff, tileIndex, tileData.data(), tileSize) < 0) {
					cerr << "Error reading tile " << tileIndex << endl;
					TIFFClose(inTiff);
					return 1;
				}

				for (uint32_t tileY = 0; tileY < tileLength; ++tileY) {
					uint32_t dstY = y * tileLength + tileY;
					if(dstY >= height)
						break;
					for (uint32_t tileX = 0; tileX < tileWidth; ++tileX) {
						uint32_t srcIndex = tileY * tileWidth + tileX;
						uint32_t dstX = x * tileWidth + tileX;

						if (dstX >= width) {
							continue;
						}

						uint32_t dstIndex = (dstY * w + dstX);
						values[dstIndex] = tileData[srcIndex];
					}
				}
			}
		}
	}
	if(bitsPerSample==1){
		unsigned char * tileData= new unsigned char [tileSize];

		for (uint32_t y = 0; y < numTilesY; ++y) {
			for (uint32_t x = 0; x < numTilesX; ++x) {
				uint32_t tileIndex = TIFFComputeTile(inTiff, x * tileWidth, y * tileLength, 0, 0);

				if (TIFFReadEncodedTile(inTiff, tileIndex, tileData, tileSize) < 0) {
					cerr << "Error reading tile " << tileIndex << endl;
					TIFFClose(inTiff);
					return 1;
				}

				for (uint32_t tileY = 0; tileY < tileLength; ++tileY) {
					uint32_t dstY = y * tileLength + tileY;
					if(dstY >= height)
						break;
					for (uint32_t tileX = 0; tileX < tileWidth; ++tileX) {
						uint32_t srcIndex = tileY * tileWidth + tileX;
						uint32_t dstX = x * tileWidth + tileX;

						if (dstX >= width) {
							continue;
						}
						uint32_t bytePos = srcIndex >> 3;
						uint32_t bitPos = srcIndex & 7;
						uint32_t dstIndex = (dstY * w + dstX);

						values[dstIndex] = (tileData[bytePos] & (1 << bitPos)) != 0;
					}
				}
			}
		}
		delete[] tileData;
	}
	return true;
}

bool Depthmap::loadStripedTiff(TIFF* inTiff, std::vector<float> &values, uint32_t& w, uint32_t& h, uint32_t bitsPerSample){

	tsize_t scanLineSize = TIFFScanlineSize(inTiff);
	if (scanLineSize == 0) {
		cerr << "Error computing strip size." << endl;
		TIFFClose(inTiff);
		return false;
	}

	// Temporary buffer for reading one strip of data
	if(bitsPerSample==32) {
		vector<float> stripData(scanLineSize / sizeof(float));

		for (uint32_t row = 0; row < h; ++row) {
			// Read the current strip
			if (TIFFReadScanline(inTiff, stripData.data(), row, 1) < 0) {
				cerr << "Error reading strip " << row << endl;
				TIFFClose(inTiff);
				return false;
			}

			for (uint32_t col = 0; col < w; ++col) {
				uint32_t dstIndex = row * w + col;
				values[dstIndex] = stripData[col];
			}
		}
	}
	if(bitsPerSample==1) {
		unsigned char * stripData= new unsigned char [scanLineSize];

		for (uint32_t row = 0; row < h; ++row) {
			// Read the current strip
			if (TIFFReadScanline(inTiff, stripData, row) < 0) {
				cerr << "Error reading strip " << row << endl;
				TIFFClose(inTiff);
				return false;
			}

			for (uint32_t col = 0; col < w; ++col) {
				uint32_t bytePos = col >> 3;
				uint32_t bitPos = 7 - (col & 7);
				uint32_t dstIndex = row * w + col;


				values[dstIndex] = (stripData[bytePos] & (1 << bitPos)) != 0;
				cout << values[dstIndex];
			}
			cout << endl;
		}
		delete[] stripData;
	}
	return true;
}

bool Depthmap::load(const char *tiff) {
	if (!loadTiff(tiff, elevation, width, height)) {
		cerr << "Failed to load TIFF file: " << tiff << endl;
		return false;
	}
	QString tiffPath = QString(tiff);
	QString xmlPath = tiffPath.left(tiffPath.lastIndexOf('.')) + ".xml";

	if (!loadXml(xmlPath)) {
		cerr << "Failed to load XML file: " << xmlPath.toStdString() << endl;
		return false;
	}

	return true;
}

bool Depthmap::loadXml(const QString &xmlPath){
	//depthmap in Malt Z deZoom ecc
	QFile file(xmlPath);
	if (!file.open(QIODevice::ReadOnly)) {
		cerr << "Cannot open XML file: " << xmlPath.toStdString() << endl;
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

void Depthmap::computeNormals() {

	normals.resize(width*height);

	for(uint32_t y = 0; y < height; y++) {
		for(uint32_t x = 0; x < width; x++) {
			uint32_t i = x + y*width;
			Eigen::Vector3f &n = normals[i];
			float d = 2.0f;

			float left;
			if(x > 0) {
				left = elevation[i-1];
			} else {
				d = 1.0f;
				left = elevation[i];
			}

			float right;
			if(x < width-1) {
				right = elevation[i+1];
			} else {
				d = 1.0f;
				right = elevation[i];
			}
			n[0] = -(right - left)/d;

			d = 2.0f;

			float top;
			if(y > 0) {
				top = elevation[i-width];
			} else {
				d = 1.0f;
				top = elevation[i];
			}

			float bottom;
			if(y < height-1) {
				bottom = elevation[i+width];
			} else {
				d = 1.0f;
				bottom = elevation[i];
			}
			n[1] = -(top - bottom)/d;
			n[2] = 1.0f;
			n.normalize();

		}
	}
}

void Depthmap::projectToCameraDepthMap(const Camera& camera, const QString& outputPath) {

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

bool Depthmap::loadMask(const char *tifPath){
	//loaded masq orthoplane MicMac
	uint32_t w, h;
	if (!loadTiff(tifPath, mask, w, h)) {
		cerr << "Failed to load TIFF file: " << tifPath << endl;
		return false;
	}
	if(width != w || height != h){
		cerr << "Mask is not consistent with height or width" << endl;
		return false;
	}

	return true;

}

void Depthmap::depthIntegrateNormals(){
	if (normals.empty()){
		cerr << "Error: no normals found" << endl;
	}
	std::function<bool(std::string s, int d)> callback = [this](std::string s, int n)->bool { return true; };
	vector<float> normals_float(normals.size()*3);
	for(size_t i = 0; i < normals.size(); i++)
		for(int k = 0; k < 3; k++)
			normals_float[i*3+k] = normals[i][k];
	elevation.clear();
	elevation.resize(normals.size(), 0);
	bni_integrate(callback, width, height, normals_float, elevation);
	for(float &h: elevation){
		h = -h;
	}
}
void Depthmap::resizeNormals (int factorPowerOfTwo, int step = 1) {
	int factor = 1 << factorPowerOfTwo;
	int targetWidth = width/factor;
	int targetHeight = height/factor;

	std::vector<float> dxArray(targetWidth * targetHeight, 0.0f);
	std::vector<float> dyArray(targetWidth * targetHeight, 0.0f);


	std::vector<Eigen::Vector3f> resizedNormals(targetWidth * targetHeight);
	cout << "Resizing normals: Factor = " << factor << ", Target Width = " << targetWidth << ", Target Height = " << targetHeight << endl;

	for (int y = 0; y < targetHeight; y++) {
		for (int x = 0; x < targetWidth; x++) {
			//	Eigen::Vector2f avgDelta = Eigen::Vector2f::Zero();
			float sumDx = 0.0f;
			float sumDy = 0.0f;

			float w = 0;
			for (int dy = 0; dy < factor; dy += step) {
				for (int dx = 0; dx < factor; dx += step) {
					int srcX = x * factor + dx;
					int srcY = y * factor + dy;

					if (srcX < width && srcY < height) {

						// 1. create the array dx and dy, after convert to normals
						Eigen::Vector3f normal = normals[srcX + srcY * width];
						float dzdx = -normal.x() / normal.z();
						float dzdy = -normal.y() / normal.z();

						sumDx += dzdx;
						sumDy += dzdy;
						w++;
					}
				}
			}
			float avgDx = sumDx / w;
			float avgDy = sumDy / w;
			Eigen::Vector3f normal(avgDx, avgDy, 1.0f);
			normal.normalize();
			resizedNormals[y * targetWidth + x] = normal;
		}
	}

	/*
			resizedNormals[y * targetWidth + x] = avgNormal / (factor * factor);
		}*/

	normals = resizedNormals;
	width = targetWidth;
	height = targetHeight;
	resolution *= factor;

	//QString filename = "/Users/erika/Desktop/testcenterRel_copia/photogrammetry/surface.jpg";
	//saveNormals(filename.toStdString().c_str());
	//depthIntegrateNormals();

}

void Depthmap::saveNormals(const char *filename) {

	QImage img(width, height, QImage::Format::Format_ARGB32);

	for(uint32_t y = 0; y < height; y++) {
		for(uint32_t x = 0; x < width; x++) {
			Eigen::Vector3f n = normals[x + y*width];
			int r = (int)round(((n[0] + 1.0f)/2.0f)*255.0f);
			int g = (int)round(((n[1] + 1.0f)/2.0f)*255.0f);
			int b = (int)round(((n[2] + 1.0f)/2.0f)*255.0f);
			img.setPixel(x, y, qRgb(r, g, b));
		}
	}
	img.save(filename);
}
void Depthmap::saveObj(const char *filename){
	//1. usa QFile per scrivere il file e QTextStream
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
			//3. obj coordinate di un punto v primacord v secondacord ecc. e poi esci chiama nel main
			out << "v " << realPos.x() << " " << realPos.y() << " " << realPos.z() << "\n";
		}
	}
}

//prendi x=160,14 e y=140 dell'img
//le coordinate del passo del pixel 0,016 sono nel xml della depth map Z_num ecc.

Eigen::Vector3f Depthmap::pixelToRealCoordinates(int pixelX, int pixelY, float pixelZ) {

	// converto in punti 3d. origine dell'img + passoX * 160x
	float realX = origin[0] + resolution[0] * pixelX;
	float realY = realY = origin[1] + resolution[1] * pixelY;
	float realZ = origin[2] + resolution[2] * pixelZ;

	return Eigen::Vector3f(realX, realY, realZ);
}

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
	QFileInfo fileInfo(fileInterneNode.text());
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
		cerr << "Warning: Z è zero, impossibile proiettare il punto." << endl;
		return Eigen::Vector3f(0, 0, 0);
	}
	//Normalizza dividendo per la coordinata z per ottenere le coordinate immagine u e  v come coordinate 2D proiettate
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

Eigen::Vector3f Camera::applyIntrinsicCalibration(Eigen::Vector3f& uvz) const{

	float u = uvz.x();
	float v = uvz.y();
	Eigen::Vector2f P(PPx, PPy);
	Eigen::Vector2f UV(u, v);
	Eigen::Vector2f result = P + focal * UV;

	return Eigen::Vector3f(result.x(), result.y(), uvz.z());
}
// du =U−Cx dv =V −Cy ρ2 =d2u +d2v
// DRUV=Cx+(1+R3ρ2 +R5ρ4 +R7ρ6)du

Eigen::Vector3f Camera::applyRadialDistortion(Eigen::Vector3f& uvz) {

	float du = uvz.x() - Cx;
	float dv = uvz.y() - Cy;
	float rho2 = du * du + dv * dv;

	float distortionFactor = 1 + R3 * rho2 + R5 * std::pow(rho2, 2) + R7 * std::pow(rho2, 3);

	float u_dist = Cx + distortionFactor * du;
	float v_dist = Cy + distortionFactor * dv;

	return Eigen::Vector3f(u_dist, v_dist, uvz.z());
}