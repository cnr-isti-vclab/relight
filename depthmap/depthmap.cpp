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

/* ---------------------------------------------------------------------------------------------------------------------------*/
//start depthmap class


//Create a function that reads the tiffs, then create two functions that read the tileTiffs and the stripTiffs
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
	if(samplesPerPixel !=1) {
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
	TIFFClose(inTiff);
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
			}
		}
		delete[] stripData;
	}
	TIFFClose(inTiff);
	return true;
}

bool Depthmap::loadDepth(const char *tiff) {
	if (!loadTiff(tiff, elevation, width, height)) {
		cerr << "Failed to load depth TIFF file: " << tiff << endl;
		return false;
	}
	return true;
}

bool Depthmap::loadMask(const char *tifPath){
	//loaded masq orthoplane MicMac
	uint32_t w, h;
	if (!loadTiff(tifPath, mask, w, h)) {
		cerr << "Failed to load mask TIFF file: " << tifPath << endl;
		return false;
	}
	if(width != w || height != h){
		cerr << "Mask is not consistent with height or width" << endl;
		return false;
	}

	return true;

}

//1. scrivere la depth map e la masq fai tif .save con i nomi.
//2. prendi la superficie dell rti e trovi la media delle due superfici della depth e della rti. sottraiamo la media del rti per vedere se hanno la stessa media.
//void Depthmap::

bool Depthmap::loadNormals(const char *normals_path){

	QImage normalmap(normals_path);
	if (normalmap.isNull()) {
		cerr << "Failed to load normalmap file: " << normals_path << endl;
		return false;
	}

	width = normalmap.width();
	height = normalmap.height();

	normals.resize(width * height);

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			QRgb rgb = normalmap.pixel(x, y);
			int i = x + y * width;

			normals[i] = Eigen::Vector3f(
						(qRed(rgb) / 255.0f) * 2.0f - 1.0f,
						(qGreen(rgb) / 255.0f) * 2.0f - 1.0f,
						(qBlue(rgb) / 255.0f) * 2.0f - 1.0f
						);
		}
	}

	return true;
}
void Depthmap::saveTiff(const char *mask_path,const vector<float> &values, uint32_t w, uint32_t h, uint32_t bitsPerSample) const{
	//save e scrive la maschera

	TIFF* maskTiff = TIFFOpen(mask_path, "w");
	if (!maskTiff) {
		cerr << "Could not open mask TIFF file for writing: " << mask_path << endl;
		return;
	}

	TIFFSetField(maskTiff, TIFFTAG_IMAGEWIDTH, w);
	TIFFSetField(maskTiff, TIFFTAG_IMAGELENGTH, h);
	TIFFSetField(maskTiff, TIFFTAG_SAMPLESPERPIXEL, 1);

	if (bitsPerSample == 32) {
		TIFFSetField(maskTiff, TIFFTAG_BITSPERSAMPLE, bitsPerSample);
		TIFFSetField(maskTiff, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
		TIFFSetField(maskTiff, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
	} else if (bitsPerSample == 1) {
		TIFFSetField(maskTiff, TIFFTAG_BITSPERSAMPLE, 1);
		TIFFSetField(maskTiff, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
		TIFFSetField(maskTiff, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
	}



	tsize_t scanLineSize = TIFFScanlineSize(maskTiff);
	if (scanLineSize == 0) {
		cerr << "Error computing scanline size." << endl;
		TIFFClose(maskTiff);
		throw QString("Width is zero");
	}


	for (uint32_t y = 0; y < h; ++y) {
		if (bitsPerSample == 32) {

			std::vector<float> stripData(scanLineSize / sizeof(float));
			for (uint32_t x = 0; x < w; ++x) {
				uint32_t index = y * w + x;
				stripData[x] = values[index];
			}

			if (TIFFWriteScanline(maskTiff, stripData.data(), y, 0) < 0) {
				cerr << "Error writing scanline " << y << " for mask." << endl;
				TIFFClose(maskTiff);
				return;
			}
		}
		if (bitsPerSample == 1) {

			unsigned char * scanline= new unsigned char [scanLineSize];

			memset(scanline, 0, scanLineSize);
			// write the current strip
			for (uint32_t col = 0; col < w; ++col) {
				uint32_t bytePos = col >> 3;
				uint32_t bitPos = 7 - (col & 7);
				uint32_t dstIndex = y * w + col;

				//values[dstIndex] = (scanline[bytePos] & (1 << bitPos)) != 0;
				if(values[dstIndex] != 0.0f){
					scanline[bytePos] |= (1 << bitPos);
				}
			}
			if (TIFFWriteScanline(maskTiff, scanline, y, 0) < 0) {
				cerr << "Error writing strip " << y << endl;
				TIFFClose(maskTiff);
				return;
			}

			delete[] scanline;
		}


	}
	TIFFClose(maskTiff);
}


void Depthmap::saveDepth(const char *depth_path) const{
	saveTiff(depth_path, elevation, width, height, 32);
}

void Depthmap::saveMask(const char *mask_path){
	saveTiff(mask_path, mask, width, height, 1);
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

//mean depth
float Depthmap::calculateMeanDepth(const std::vector<float>& values) {
	if (elevation.empty()) return 0.0f;
	return std::accumulate(values.begin(), values.end(), 0.0f) / values.size();
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


void Depthmap::resizeNormals (int factorPowerOfTwo, int step) {
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

						// 1. create the array dx and dxy, after convert to normals
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

	//saveNormals(filename.toStdString().c_str());
	//depthIntegrateNormals();

}


/*

void Depthmap::depth(const char *depth_path){
	std::vector<float> depthValues;
	if (!load(depth_path)) {
		cerr << "Failed to load depth map." << endl;
		return;
	}
	for (size_t i = 0; i < depthValues.size(); i++) {
		float realX = depthValues[i];
		float realY = depthValues[++i];
		float realZ = depthValues[++i];

		Eigen::Vector3f pixelCoords = realToPixelCoord(realX, realY, realZ);
		int pixelX = pixelCoords[0];
		int pixelY = pixelCoords[1];
		float calcZ = pixelCoords[2];

		Eigen::Vector3f  = projectToCameraDepthMap(pixelCoord);

	}

}

//funzione inversa da 3d a xyh; xy z=0 h? pixel,pixel, h
void Depthmap::t3DToCamera(float h, ){
	//xyz coord 3d fai l'inversa: aperi txt, x e y. prendi funz 3d la trosformi in x y e h e poi applichi quella già fatta camerato3d e deve venire uguale
	//poi prendi apericloud e vedi se coincide con la depth
	//2. scrivi la funzione inversa da un punto in 3d ci trova la x e la y in pixel e la h dalla depth del mic mac
	//3. verifica se la h trovata per i punti di aperi corrisponde all h della depth map

	for (size_t i = 0; i < points.size(); i += 3) {
		float realX = points[i];
		float realY = points[i + 1];
		float realZ = points[i + 2];

		Eigen::Vector3f pixelCoords = realToPixelCoordinates(realX, realY, realZ);

		int pixelX = pixelCoords[0];
		int pixelY = pixelCoords[1];
		float calcZ = pixelCoords[2];


		float invH= (h - origin[2]) / resolution[2];
	}
}

*/
//take the x=160,14 e y=140
//the coordinates of the pixel step 0.016 are in the depth map xml Z_num etc.

/* ---------------------------------------------------------------------------------------------------------------------------*/
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
	gaussianGrid.minSamples = 3; // 1, 3, 5
	gaussianGrid.sideFactor = 1; // 0.25, 0.5, 1
	gaussianGrid.init(imageCloud, source);
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
/*_-----------------------------------------------------------------------------------------*/
void GaussianGrid::fitLinear(std::vector<float> &x, std::vector<float> &y, float &a, float &b) {
	if (x.size() != y.size()) {
		cout << "Errore: i vettori x e y devono avere la stessa lunghezza." << endl;
		return;
	}

	int n = x.size();
	float sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_x2 = 0.0;

	for (int i = 0; i < n; i++) {
		sum_x += x[i];
		sum_y += y[i];
		sum_xy += x[i] * y[i];
		sum_x2 += x[i] * x[i];
	}

	// Calcolo dei coefficienti a e b
	a = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
	b = (sum_y - a * sum_x) / n;
}

float GaussianGrid::bilinearInterpolation(float x, float y) {

	float x1 = floor(x);
	float y1 = floor(y);
	float x2 = x1+1;
	float y2 = y1+1;


	if (x1 < 0 || x2 >= width || y1 < 0 || y2 >= height) {
		cerr << "Coordinate fuori dai limiti della griglia!" << endl;
		return 0.0f;
	}


	float Q11 = values[x1 + y1 * width];
	float Q12 = values[x1 + y2 * width];
	float Q21 = values[x2 + y1 * width];
	float Q22 = values[x2 + y2 * width];

	float R1 = (x2 - x) * Q11 + (x - x1) * Q21;
	float R2 = (x2 - x) * Q12 + (x - x1) * Q22;

	float P = (y2 - y) * R1 + (y - y1) * R2;

	return P;
}
//fit h = a+b*elev
void GaussianGrid::init(std::vector<Eigen::Vector3f> &cloud, std::vector<float> &source) {
	int side = static_cast<int>(sideFactor * sqrt(cloud.size()));
	sigma = 1.0f / side;
	width = side;
	height = side;
	float precision = 0.00001f;

	vector<float> cloudZ;
	for(auto &v: cloud)
		cloudZ.push_back(v[2]);

	fitLinear(source, cloudZ, a, b);
	//TODO: w and h proportional to aspect ratio of camera
	for(size_t i = 0; i < cloud.size(); i++) {
		cloud[i][2] -= depthmapToCloud(source[i]);
	}

	computeGaussianWeightedGrid(cloud);
	fillLaplacian(precision);
}

void GaussianGrid::fillLaplacian(float precision){
	float mean = 0;
	float count = 0;
	for(int i = 0; i < values.size(); i++){
		if(weights[i] != 0){
			mean += values[i];
			count++;
		}
	}
	mean/=count;
	for(int i = 0; i < values.size(); i++){
		if(weights[i] == 0){
			values[i] = mean;
		}
	}

	std::vector<float> old_values = values;
	std::vector<float> new_values = values;

	bool converged = false;
	float max_iter = 1000;

	for (int i = 0; i < max_iter; i++) {

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; ++x) {
				int index = y * width + x;

				if (weights[index] > 0) {
					continue;
				}

				float sum_neighbors = 0.0f;
				int count_neighbors = 0;

				//neighbors up
				if (y > 0) {
					sum_neighbors += old_values[(y - 1) * width + x];
					count_neighbors++;
				}

				// neigh down
				if (y < height - 1) {
					sum_neighbors += old_values[(y + 1) * width + x];
					count_neighbors++;
				}

				// neigh left
				if (x > 0) {
					sum_neighbors += old_values[y * width + (x - 1)];
					count_neighbors++;
				}

				// neigh right
				if (x < width - 1) {
					sum_neighbors += old_values[y * width + (x + 1)];
					count_neighbors++;
				}

				// update the value
				if (count_neighbors > 0) {
					new_values[index] = sum_neighbors / count_neighbors;
				}
			}
		}

		float max_difference = 0.0f;
		for (size_t i = 0; i < values.size(); ++i) {
			max_difference = std::max(max_difference, std::abs(new_values[i] - old_values[i]));
		}

		if (max_difference < precision) {
			qDebug() << "Converged after" << i + 1 << "iterations.";
			converged = true;
			break;
		}

		old_values = new_values;
	}

	if (!converged) {
		qDebug() << "Reached maximum iterations without full convergence.";
	}

	values = new_values;
}

float GaussianGrid::value(float x, float y){
	//bicubic interpolation
	//nearest
	float pixelX = x * (width-1);
	float pixelY = y * (height-1);

	return bilinearInterpolation(pixelX, pixelY);

	//return values[pixelX + pixelY * width];

}

float GaussianGrid::target(float x, float y, float h) {
	h = depthmapToCloud(h);
	return h + value(x, y);
}

void GaussianGrid::computeGaussianWeightedGrid(std::vector<Eigen::Vector3f> &differences) {

	float x_min = 0;
	float x_max = 1;
	float y_min = 0;
	float y_max = 1;

	float x_step = (x_max - x_min) / (width - 1);
	float y_step = (y_max - y_min) / (height - 1);

	values.resize(width * height, 0);
	weights.resize(width * height, 0);

	std::vector<int> count(width*height, 0);


	float max_distance = 2 * sigma;
	for (auto &p : differences) {


		int x_start = std::max(0, static_cast<int>((p[0] - max_distance - x_min) / x_step));
		int x_end = std::min(width - 1, static_cast<int>((p[0] + max_distance - x_min) / x_step));
		int y_start = std::max(0, static_cast<int>((p[1] - max_distance - y_min) / y_step));
		int y_end = std::min(height - 1, static_cast<int>((p[1] + max_distance - y_min) / y_step));

		for (int x = x_start; x <= x_end; x++) {
			for (int y = y_start; y <= y_end; y++) {
				float xg = x_min + x * x_step;
				float yg = y_min + y * y_step;

				float distance = sqrt((p[0] - xg) * (p[0] - xg) + (p[1] - yg) * (p[1] - yg));
				if (distance <= max_distance) {
					float weight = exp(-(distance * distance) / (2 * sigma * sigma));
					values[y * width + x] += weight * p[2];
					weights[y * width + x] += weight;
					count[y*width + x]++;
				}
			}
		}
	}
	//chiama camere tutte e 4 e vedi come vengono
	// pesare per il blanding funzione intervallo 0, 1 * 0, 1 0 ai bordi 1 al centro, che sia una funzione continua
	//polinomio di 2 grado in x * pol 2 grado in y. derivata e peso a 0 sul bordo
	//fai somma pesata e veedi come vieni
	//funz target ritorna valore e peso

	for (int i = 0; i < values.size(); i++) {
		if(count[i] < minSamples)
			weights[i] = 0;
		if (weights[i] != 0) {
			values[i] /= (weights[i]);
		}
	}
	/*for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			qDebug() << "z_grid[" << i << "][" << j << "] = " << values[i * height +j];
		}
	}*/
}
//se < 1/4 è -8x^2, else se è < 3/4. 8*(x-1)^2, data una x mi ritorna una x+1

static float bell(float x){
	if(x < 0.25f){
		return 8.0f* x * x;
	}
	else if(x< 0.75f){
		return -8.0f * x *x + 8.0f * x-1.0f;
	}
	else {
		x = 1.0f - x;
		return 8.0f * x * x;
	}

}

float Depthmap::calculateWeight(float x, float y) const{

	x /= width;
	y /= height;

	//	float weightX =pow(cos(M_PI * (x-0.5f)), 2);
	//float weightY = pow(cos(M_PI * (y-0.5f)), 2);
	return bell(x)*bell(y);
	//return weightX * weightY;
}

void GaussianGrid::imageGrid(const char* filename) {

	auto min_max = minmax_element(values.begin(), values.end());
	float z_min = *min_max.first;
	float z_max = *min_max.second;

	if (z_max == z_min) {
		cerr << "All values in z_grid are the same. Cannot normalize." << endl;
		return;
	}

	QImage image(width, height, QImage::Format_Grayscale8);
	if (!QFile::exists(QFileInfo(filename).absolutePath())) {
		qDebug() << "Directory does not exist: " << QFileInfo(filename).absolutePath();
		return;
	}

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			float value = values[i * width + j];
			float normalized_value = (value - z_min) / (z_max - z_min);
			int gray_value = static_cast<int>(normalized_value * 255);
			image.setPixel(j, i, qRgb(gray_value, gray_value, gray_value));
		}
	}
	image.save(filename, "png");
}




//scala tra 0 e 1. img



//Real to Pixel Coordinates: (-0.981, 2.041, -11.132) -> Pixel: (110.688, 65.4375, -27.75)
//point outside the image limits Point 3D: (-0.981, 2.041, -11.132), Coordinate pixel: (-1, 2)


/* ---------------------------------------------------------------------------------------------------------------------------*/


