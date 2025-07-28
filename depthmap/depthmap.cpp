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
//#include "../src/flatten.h"
//#include <assm/Grid.h>

//carica la mask, applica il gaussian blur con il peso scalato da 0.5 a 1 fai la somma pesata tra la depth di Mic Mac e quella ricavata dal gaussian blur
//gaussian, maschera 0.5 1, peso mediato
using namespace std;


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
	std::function<bool(QString s, int d)> callback = [this](QString s, int n)->bool { return true; };
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




//Real to Pixel Coordinates: (-0.981, 2.041, -11.132) -> Pixel: (110.688, 65.4375, -27.75)
//point outside the image limits Point 3D: (-0.981, 2.041, -11.132), Coordinate pixel: (-1, 2)


/* ---------------------------------------------------------------------------------------------------------------------------*/


