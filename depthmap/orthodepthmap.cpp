#include "orthodepthmap.h"
#include "camera.h"
#include <tiffio.h>
#include <iostream>
#include <cmath>
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

bool OrthoDepthmap::load(const char *depth_path, const char *mask_path, const char *correlation_path){

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

	QString qcorrelation_path = QString(correlation_path);
	if(!loadCorrelation(qcorrelation_path.toStdString().c_str())){
		cerr << "Failed to load ortho mask tiff file: " << qcorrelation_path.toStdString() << endl;
		return false;
	}

	QString xmlPath = qdepth_path.left(qdepth_path.lastIndexOf('.')) + ".xml";
	if (!loadXml(xmlPath.toStdString().c_str())) {
		cerr << "Failed to load XML file: " << xmlPath.toStdString() << endl;
		return false;
	}
	return true;

}

bool OrthoDepthmap::loadCorrelation(const char *tifPath){
	//loaded masq orthoplane MicMac
	uint32_t w, h;
	if (!loadTiff(tifPath, correlation, w, h)) {
		throw QString("Failed to load correlation TIFF file: ") + tifPath;
	}

	if(width != w || height != h){
		throw QString("Mask is not consistent with height or width");
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

	// Add faces (triangles for grid)
	for (uint32_t y = 0; y < height - 1; y++) {
		for (uint32_t x = 0; x < width - 1; x++) {
			// Check if all 4 corners of the quad are valid
			uint32_t idx00 = x + y * width;
			uint32_t idx10 = (x + 1) + y * width;
			uint32_t idx01 = x + (y + 1) * width;
			uint32_t idx11 = (x + 1) + (y + 1) * width;
			
			if (mask[idx00] > 0 && mask[idx10] > 0 && mask[idx01] > 0 && mask[idx11] > 0) {
				// OBJ indices are 1-based
				uint32_t v00 = idx00 + 1;
				uint32_t v10 = idx10 + 1;
				uint32_t v01 = idx01 + 1;
				uint32_t v11 = idx11 + 1;
				
				// Write two triangles for the quad
				out << "f " << v00 << " " << v01 << " " << v10 << "\n";
				out << "f " << v10 << " " << v01 << " " << v11 << "\n";
			}
		}
	}

	
}

#include <Eigen/Dense>
#include <cmath>

// Standard 2D Gaussian spatial filter for Eigen Matrix (Depth Map)
Eigen::MatrixXf gaussianBlur(const Eigen::MatrixXf& src, float sigma) {
	int radius = std::ceil(3.0f * sigma);
	int kernelSize = 2 * radius + 1;
	Eigen::VectorXf kernel(kernelSize);
	for (int i = -radius; i <= radius; ++i) {
		kernel(i + radius) = std::exp(-0.5f * (i * i) / (sigma * sigma));
	}
	kernel /= kernel.sum(); // Normalize

	Eigen::MatrixXf blurX = Eigen::MatrixXf::Zero(src.rows(), src.cols());
	Eigen::MatrixXf blurXY = Eigen::MatrixXf::Zero(src.rows(), src.cols());

	// Separable 1D convolutions
	for (int
			 r = 0; r < src.rows(); ++r)
		for (int c = 0; c < src.cols(); ++c)
			for (int k = -radius; k <= radius; ++k)
				blurX(r, c) += src(r, std::clamp(c + k, 0, (int)src.cols() - 1)) * kernel(k + radius);

	for (int r = 0; r < src.rows(); ++r)
		for (int c = 0; c < src.cols(); ++c)
			for (int k = -radius; k <= radius; ++k)
				blurXY(r, c) += blurX(std::clamp(r + k, 0, (int)src.rows() - 1), c) * kernel(k + radius);

	return blurXY;
}

#include <Eigen/Dense>
#include <vector>
#include <algorithm>
#include <cmath>

// Estimates global scale alpha using long-span gradients
float estimateScaleLongSpanGradients(
	const Eigen::MatrixXf& zPS,
	const Eigen::MatrixXf& zSfM,
	int spanPixels = 25,          // Half-span d (full span = 2 * d = 50 pixels)
	float minGradientThreshold = 0.005f // Ignores flat regions to prevent divide-by-zero
	) {
	const int rows = zPS.rows();
	const int cols = zPS.cols();

	// Ensure image is larger than double the half-span
	if (rows <= 2 * spanPixels || cols <= 2 * spanPixels) {
		return 1.0f;
	}

	std::vector<float> ratios;
	ratios.reserve((rows - 2 * spanPixels) * (cols - 2 * spanPixels));

	const float normFactor = 1.0f / (2.0f * spanPixels);

	for (int r = spanPixels; r < rows - spanPixels; ++r) {
		for (int c = spanPixels; c < cols - spanPixels; ++c) {

			// 1. Long-span central differences for PS
			float gx_PS = (zPS(r, c + spanPixels) - zPS(r, c - spanPixels)) * normFactor;
			float gy_PS = (zPS(r + spanPixels, c) - zPS(r - spanPixels, c)) * normFactor;
			float magPS = std::sqrt(gx_PS * gx_PS + gy_PS * gy_PS);

			// 2. Long-span central differences for SfM
			float gx_SfM = (zSfM(r, c + spanPixels) - zSfM(r, c - spanPixels)) * normFactor;
			float gy_SfM = (zSfM(r + spanPixels, c) - zSfM(r - spanPixels, c)) * normFactor;
			float magSfM = std::sqrt(gx_SfM * gx_SfM + gy_SfM * gy_SfM);

			// 3. Filter out flat/textureless areas where ratios become unstable
			if (magSfM > minGradientThreshold && magPS > minGradientThreshold) {
				ratios.push_back(magSfM /magPS);
			}
		}
	}

	if (ratios.empty()) {
		return 1.0f;
	}

	// 4. Return Median Ratio (robust to outliers and local detail mismatches)
	size_t mid = ratios.size() / 2;
	std::nth_element(ratios.begin(), ratios.begin() + mid, ratios.end());
	return ratios[mid];
}

// Fuses PS and SfM depth maps using frequency-band separation
Eigen::MatrixXf fuseDepthFrequencySplit(
	const Eigen::MatrixXf& zPS,   // High-detail PS Depth
	const Eigen::MatrixXf& zSfM,  // Scaled/projected SfM Depth (unknown zero offset)
	float sigma = 100.0f            // Filter radius tuning high vs low frequency cutoff
	) {
	// 1. Extract Low Frequencies (L) via Gaussian Low-Pass Filter
	Eigen::MatrixXf L_PS  = gaussianBlur(zPS, sigma);
	Eigen::MatrixXf L_SfM = gaussianBlur(zSfM, sigma);

	// 2. Extract High Frequency detail layer (H) from PS (Invariant to offset)
	Eigen::MatrixXf H_PS = zPS - L_PS;

	// 3. Compute scale alpha via standard deviation ratio (Offset invariant: Var(X + C) = Var(X))
	//	float stdPS  = std::sqrt((L_PS.array()  - L_PS.mean()).square().sum()  / L_PS.size());
	//	float stdSfM = std::sqrt((L_SfM.array() - L_SfM.mean()).square().sum() / L_SfM.size());
	//	float alpha  = stdSfM/stdPS ;
	float alpha = estimateScaleLongSpanGradients(zPS,zSfM);
	cout << "Alpha: " << alpha << endl;
	// 4. Fuse: Anchored Low-Freq SfM baseline + Unmodified High-Freq PS details
	return L_SfM + (alpha * H_PS);
}

// Fill holes in depth map using iterative neighbor propagation
void fillDepthMapHoles(std::vector<float>& elevation, std::vector<float>& correlation, std::vector<float>& mask, int width, int height) {
	const int maxIterations = 10;
	for (int iter = 0; iter < maxIterations; iter++) {
		bool anyFilled = false;
		std::vector<float> newElevation = elevation;
		std::vector<float> newMask = mask;

		for (int y = 1; y < height - 1; y++) {
			for (int x = 1; x < width - 1; x++) {
				int idx = x + y * width;
				if (mask[idx] > 0.0f)
					continue;

				// Check 4-connected neighbors
				int neighbors[4] = {
					(x - 1) + y * width,
					(x + 1) + y * width,
					x + (y - 1) * width,
					x + (y + 1) * width
				};


				float corr = 0.0f;
				float sum = 0.0f;
				int count = 0;
				for (int n : neighbors) {
					if (mask[n] > 0.0f) {
						sum += elevation[n];
						corr += correlation[n];
						count++;
					}
				}

				if (count > 0) {
					correlation[idx] = corr/count;
					newElevation[idx] = sum/count;
					newMask[idx] = 0.5f; // Mark as interpolated
					anyFilled = true;
				}
			}
		}

		elevation = newElevation;
		mask = newMask;

		if (!anyFilled)
			break;
	}
}

void OrthoDepthmap::projectToCameraDepthMap(const CameraDepthmap& cameradepth, const QString& outputPath) {
	auto &camera = cameradepth.camera;

	// Initialize mask and elevation arrays for the camera depth
	const std::vector<float> &depthElevation = cameradepth.elevation;
	const std::vector<float> &depthMask = cameradepth.mask;
	int depthSize = camera.width * camera.height;

	Depthmap projected;
	// Project ortho depth onto camera image, storing directly in depthmap
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if(mask[x + y * width] == 0.0f)
				continue;
			float pixelZ = old_elevation[x + y * width];

			Eigen::Vector3f realCoordinates = pixelToRealCoordinates(x, y, pixelZ);
			Eigen::Vector3f imageCoords = camera.projectionToImage(realCoordinates);

			int imageX = (int)round(imageCoords[0]);
			int imageY = (int)round(imageCoords[1]);

			if (imageX >= 0 && imageX < camera.width && imageY >= 0 && imageY < camera.height) {
				projected.elevation[imageX + imageY * camera.width] = imageCoords[2];
				projected.mask[imageX + imageY * camera.width] = 1.0f;
			}
		}
	}

	// Fill holes in the depth map using simple iterative neighbor propagation with mask tracking
//	fillDepthMapHoles(projected.elevation, projected.mask, proj_correlation, camera.width, camera.height);

	// Fuse camera depth (high detail) with projected ortho (structure) using frequency splitting
	{
		// Convert camera depth to Eigen matrix (zPS - high detail)
		Eigen::MatrixXf zPS = Eigen::MatrixXf::Zero(camera.height, camera.width);
		for (int y = 0; y < camera.height; y++) {
			for (int x = 0; x < camera.width; x++) {
				zPS(y, x) = cameradepth.elevation[x + y * camera.width];
			}
		}

		// Convert filled projected ortho depth to Eigen matrix (zSfM - structure)
		Eigen::MatrixXf zSfM = Eigen::MatrixXf::Zero(camera.height, camera.width);
		for (int y = 0; y < camera.height; y++) {
			for (int x = 0; x < camera.width; x++) {
				zSfM(y, x) = projected.elevation[x + y * camera.width];
			}
		}

		// Fuse using frequency-band separation
		Eigen::MatrixXf fused = fuseDepthFrequencySplit(zPS, zSfM, .0f);

		// Find min/max for normalization
		float fusedMin = fused.minCoeff();
		float fusedMax = fused.maxCoeff();
		float fusedRange = fusedMax - fusedMin;
		if (fusedRange < 1e-5f) fusedRange = 1.0f;

		// Create fused depth map image and save as prova1.png
		QImage fusedImage(camera.width, camera.height, QImage::Format_RGB888);
		for (int y = 0; y < camera.height; y++) {
			for (int x = 0; x < camera.width; x++) {
				float normalized = (fused(y, x) - fusedMin) / fusedRange;
				normalized = std::max(0.0f, std::min(1.0f, normalized));
				int val = (int)(normalized * 255.0f);
				fusedImage.setPixel(x, y, qRgb(val, val, val));
			}
		}

		fusedImage.save("prova1.png");
		cout << "Fused depth map saved as prova1.png\n";
	}
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
	/*
	int cx = 892;
	int cy = 438;
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
*/



	old_elevation = elevation;
	
	// Compare point cloud with loaded elevation to validate coordinate conversion functions
	{
		std::vector<std::pair<size_t, float>> pointErrors;  // (point index, error)
		float sumError = 0.0f;
		float maxError = 0.0f;
		int validPoints = 0;
		
		for (size_t i = 0; i < point_cloud.size(); i++) {
			const auto& realCoord = point_cloud[i];
			// Convert real world coordinates to pixel/ortho coordinates
			Eigen::Vector3f pixelCoord = realToPixelCoord(realCoord[0], realCoord[1], realCoord[2]);
			
			int pixelX = static_cast<int>(std::round(pixelCoord[0]));
			int pixelY = static_cast<int>(std::round(pixelCoord[1]));
			
			// Check if pixel is within bounds
			if (pixelX >= 0 && pixelX < static_cast<int>(width) && 
				pixelY >= 0 && pixelY < static_cast<int>(height)) {
				
				float elevationZ = elevation[pixelX + pixelY * width];
				float cloudZ = pixelCoord[2];
				float error = std::abs(cloudZ - elevationZ);
				
				pointErrors.push_back({i, error});
				sumError += error;
				maxError = std::max(maxError, error);
				validPoints++;
			}
		}
		
		// Calculate average error
		float avgError = pointErrors.empty() ? 0.0f : sumError / pointErrors.size();
		
		// Calculate standard deviation
		float sumSquaredDiff = 0.0f;
		for (const auto& pe : pointErrors) {
			float diff = pe.second - avgError;
			sumSquaredDiff += diff * diff;
		}
		float stdDev = pointErrors.empty() ? 0.0f : std::sqrt(sumSquaredDiff / pointErrors.size());
		
		// Filter point cloud: keep only points with error < average error
		std::vector<Eigen::Vector3f> filteredPointCloud;
		int pointsBelowAvg = 0;
		for (const auto& pe : pointErrors) {
			if (pe.second < avgError) {
				filteredPointCloud.push_back(point_cloud[pe.first]);
				pointsBelowAvg++;
			}
		}
		
		// Replace point cloud with filtered version
		int pointsRemoved = point_cloud.size() - filteredPointCloud.size();
		point_cloud = filteredPointCloud;
		
		// Print validation report
		cout << "\n=== Coordinate Conversion Validation Report ===\n";
		cout << "Total point cloud points (original): " << (point_cloud.size() + pointsRemoved) << "\n";
		cout << "Valid points in elevation bounds: " << validPoints << "\n";
		if (!pointErrors.empty()) {
			cout << "Average elevation error: " << avgError << "\n";
			cout << "Max elevation error: " << maxError << "\n";
			cout << "Standard deviation: " << stdDev << "\n";
		}
		cout << "Points kept (error < average): " << point_cloud.size() << "\n";
		cout << "Points removed (error >= average): " << pointsRemoved << "\n";
		cout << "===============================================\n\n";
	}
	
	for(size_t i =0; i < elevation.size(); i++) {
		elevation[i] = 0.0f;
	}
	weights.clear();
	weights.resize(width * height, 0);



}



//  e un blending sulla maschera e salvare una copia dell'elevation
// in modo tale che la depth Micmac(?) prenda la depth dell rti quando è 0.5 e quando è 0 prenda la depth del micmac così da riempire i punti.
void OrthoDepthmap::endIntegration(){

	// normalize elevation by weights
	//fragni
	for (size_t i = 0; i < elevation.size(); i++) {
		if (weights[i] > 0.0f)
			elevation[i] /= weights[i];
	}

	//fill holes in elevation (weight[i] == 0 means hole)  using laplacian. (see gaussiangrid)
	//	sbertezz.

	{
		float precision = 1e-4f;
		GaussianGrid::fillLaplacian(width, height, elevation, weights, precision);

		for (size_t i = 0; i < weights.size(); ++i) {
			if (weights[i] == 0.0f && elevation[i] != 0.0f) {
				weights[i] = 1.0f;
			}
		}
	}


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
	for (int i = 0; i < width*height; ++i) {
		float v = blurred[i];
		if (v <= 0.5f) blurred_mask[i] = 0.0f;
		else blurred_mask[i] = (v - 0.5f) * 2.0f;
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
			//	sbarbugli leva questa riga che hai già normalizzato sopra.
			//	elevation[i] /= weights[i];

		}
		else {
			mask[i] = 0;
		}
		if(use_depthmap){
			elevation[i] = blur_weight * old_elevation[i] +
						   (1.0f - blur_weight) * elevation[i];
		}

	}
}

#include <Eigen/Dense>
#include <vector>
#include <random>
#include <cmath>

struct SamplePoint {
	double x, y;        // Normalized pixel/ortho coords [-1, 1]
	double zPS, zSfM;   // Depth values
};

// Write a simple SVG scatter plot of zPS (x axis) vs zSfM (y axis)
static void writeCorrelationSvg(const std::vector<SamplePoint>& samples, const std::string& filename) {
	if (samples.empty()) return;
	double minX = samples[0].zPS, maxX = samples[0].zPS;
	double minY = samples[0].zSfM, maxY = samples[0].zSfM;
	for (const auto &s : samples) {
		if (s.zPS < minX) minX = s.zPS;
		if (s.zPS > maxX) maxX = s.zPS;
		if (s.zSfM < minY) minY = s.zSfM;
		if (s.zSfM > maxY) maxY = s.zSfM;
	}

	// Add small margins
	double dx = (maxX - minX);
	double dy = (maxY - minY);
	if (dx == 0) dx = 1.0;
	if (dy == 0) dy = 1.0;
	minX -= 0.05 * dx; maxX += 0.05 * dx;
	minY -= 0.05 * dy; maxY += 0.05 * dy;

	const int W = 800;
	const int H = 800;
	const int margin = 60;

	std::ofstream out(filename);
	if (!out.is_open()) return;

	out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	out << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << W << "\" height=\"" << H << "\">\n";

	// background
	out << "<rect x=\"0\" y=\"0\" width=\"" << W << "\" height=\"" << H << "\" fill=\"white\"/>\n";

	// axes
	int ax = margin, ay = H - margin;
	int bx = W - margin, by = margin;
	// X axis line
	out << "<line x1=\"" << ax << "\" y1=\"" << ay << "\" x2=\"" << bx << "\" y2=\"" << ay << "\" stroke=\"black\" stroke-width=2/>\n";
	// Y axis line
	out << "<line x1=\"" << ax << "\" y1=\"" << ay << "\" x2=\"" << ax << "\" y2=\"" << by << "\" stroke=\"black\" stroke-width=2/>\n";

	// labels
	out << "<text x=\"" << (W/2) << "\" y=\"" << (H - 10) << "\" font-size=\"14\" text-anchor=\"middle\">zPS</text>\n";
	out << "<text transform=\"translate(15," << (H/2) << ") rotate(-90)\" font-size=\"14\" text-anchor=\"middle\">zSfM</text>\n";

	// plot points
	for (const auto &s : samples) {
		double nx = (s.zPS - minX) / (maxX - minX);
		double ny = (s.zSfM - minY) / (maxY - minY);
		double px = ax + nx * (W - 2*margin);
		double py = ay - ny * (H - 2*margin);
		out << "<circle cx=\"" << px << "\" cy=\"" << py << "\" r=\"2\" fill=\"red\" opacity=\"0.8\"/>\n";
	}

	// diagonal y=x line for reference (map min/max)
	double x0 = ax;
	double y0 = ay - ((minX - minX)/(maxX - minX)) * (H - 2*margin);
	double x1 = bx;
	double y1 = ay - ((maxX - minX)/(maxX - minX)) * (H - 2*margin);
	out << "<line x1=\"" << x0 << "\" y1=\"" << y0 << "\" x2=\"" << x1 << "\" y2=\"" << y1 << "\" stroke=\"blue\" stroke-width=\"1\" stroke-dasharray=\"4,2\"/>\n";

	out << "</svg>\n";
	out.close();
}

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <vector>
#include <cmath>
#include <iostream>

struct NormalMap {
	int width;
	int height;
	// Interleaved nx, ny, nz vectors per pixel
	std::vector<Eigen::Vector3f> data;

	const Eigen::Vector3f& at(int x, int y) const { return data[y * width + x]; }
	bool isValid(int x, int y) const {
		if (x < 0 || x >= width || y < 0 || y >= height) return false;
		const auto& n = at(x, y);
		return !std::isnan(n.z()) && std::abs(n.z()) > 1e-4f;
	}
};

// Solves the variational depth fusion problem via sparse Conjugate Gradient
Eigen::MatrixXf solvePoissonDepthFusion(
	const Eigen::MatrixXf& psNormals, // expected layout: rows=H, cols=W*3 with nx,ny,nz per pixel
	const Eigen::MatrixXf& sfmDepth,
	float scaleAlpha = 1.0f,     // Global scale factor for SfM depth
	float lambda = 0.01f          // Weighting term balancing PS details vs SfM anchors
	) {
	// infer width/height
	const int H = sfmDepth.rows();
	const int W = sfmDepth.cols();
	const int N = W * H;

	// Helper to map 2D pixel (x,y) to 1D system matrix index
	auto getIdx = [W](int x, int y) { return y * W + x; };

	// 1. Compute surface gradients p = -nx/nz, q = -ny/nz from PS normals
	// psNormals layout: rows=H, cols=W*3, columns grouped as [nx,ny,nz, nx,ny,nz, ...]
	Eigen::MatrixXf P = Eigen::MatrixXf::Zero(H, W);
	Eigen::MatrixXf Q = Eigen::MatrixXf::Zero(H, W);

	if (psNormals.rows() == H && psNormals.cols() == W * 3) {
		for (int y = 0; y < H; ++y) {
			for (int x = 0; x < W; ++x) {
				int base = x * 3;
				float nx = psNormals(y, base + 0);
				float ny = psNormals(y, base + 1);
				float nz = psNormals(y, base + 2);
				if (!std::isnan(nz) && std::abs(nz) > 1e-6f) {
					P(y, x) = -nx / nz;
					Q(y, x) = -ny / nz;
				}
			}
		}
	} else {
		// fallback: leave P,Q zero and warn
		std::cerr << "solvePoissonDepthFusion: psNormals has unexpected shape (expected H x W*3)\n";
	}

	// 2. Build Sparse System Matrix A and Right-Hand-Side vector b
	// Each pixel provides up to 4 gradient equations + 1 anchor equation
	std::vector<Eigen::Triplet<double>> triplets;
	triplets.reserve(N * 5); // Max 5 non-zero entries per row
	Eigen::VectorXd b = Eigen::VectorXd::Zero(N);

	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			int i = getIdx(x, y);


			double diagWeight = 0.0;

			// --- Horizontal Gradient Constraint: (Z(x+1, y) - Z(x, y)) = p(x, y) ---
			if (x + 1 < W) {
				int i_right = getIdx(x + 1, y);
				triplets.push_back({i, i_right, -1.0});
				diagWeight += 1.0;
				b(i) += P(y, x);
			}
			if (x - 1 >= 0) {
				int i_left = getIdx(x - 1, y);
				triplets.push_back({i, i_left, -1.0});
				diagWeight += 1.0;
				b(i) -= P(y, x - 1);
			}

			// --- Vertical Gradient Constraint: (Z(x, y+1) - Z(x, y)) = q(x, y) ---
			if (y + 1 < H) {
				int i_down = getIdx(x, y + 1);
				triplets.push_back({i, i_down, -1.0});
				diagWeight += 1.0;
				b(i) += Q(y, x);
			}
			if (y - 1 >= 0) {
				int i_up = getIdx(x, y - 1);
				triplets.push_back({i, i_up, -1.0});
				diagWeight += 1.0;
				b(i) -= Q(y - 1, x);
			}

			// --- Absolute Depth Anchor Constraint: lambda * (Z(x,y) - alpha * Z_sfm) = 0 ---
			// sfmDepth is an Eigen::MatrixXf; treat NaN as invalid
			if (x >= 0 && x < sfmDepth.cols() && y >= 0 && y < sfmDepth.rows() && !std::isnan(sfmDepth(y, x))) {
				diagWeight += lambda;
				b(i) += lambda * static_cast<double>(scaleAlpha * static_cast<double>(sfmDepth(y, x)));
			} else {
				// Minimal regularizer to prevent floating components if SfM has holes
				diagWeight += 1e-5;
			}

			triplets.push_back({i, i, diagWeight});
		}
	}

	// 3. Assemble Sparse System Matrix
	Eigen::SparseMatrix<double> A(N, N);
	A.setFromTriplets(triplets.begin(), triplets.end());
	A.makeCompressed();

	// 4. Solve System A * Z = b using Sparse Conjugate Gradient
	Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Lower | Eigen::Upper> solver;
	solver.setTolerance(1e-6);
	solver.setMaxIterations(1000);
	solver.compute(A);

	if (solver.info() != Eigen::Success) {
		throw QString("Error: Sparse matrix decomposition failed!\n");
	}

	Eigen::VectorXd zSol = solver.solve(b);

	if (solver.info() != Eigen::Success) {
		std::cerr << "Warning: Conjugate Gradient failed to converge cleanly!\n";
	}

	// 5. Package solved depth map as Eigen::MatrixXf (rows=H, cols=W)
	Eigen::MatrixXf fusedDepth = Eigen::MatrixXf::Zero(H, W);
	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			int i = y * W + x;
			fusedDepth(y, x) = static_cast<float>(zSol(i));
		}
	}

	return fusedDepth;
}

// Fits: zSfM = a * zPS + c0 + c1*x + c2*y + c3*x^2 + c4*x*y + c5*y^2
Eigen::VectorXd fitWarpModel(const std::vector<SamplePoint>& samples, const std::vector<size_t>& indices) {
	Eigen::MatrixXd A(indices.size(), 7);
	Eigen::VectorXd b(indices.size());

	for (size_t i = 0; i < indices.size(); ++i) {
		const auto& s = samples[indices[i]];
		// Design matrix row: [zPS, 1, x, y, x^2, x*y, y^2]
		A.row(i) << s.zPS, 1.0, s.x, s.y, s.x * s.x, s.x * s.y, s.y * s.y;
		b(i) = s.zSfM;
	}

	return A.bdcSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(b);
}

// Corrects PS depth map using a robust polynomial warp fitted against SfM
Eigen::MatrixXf correctPSWithPolynomialWarp(
	const Eigen::MatrixXf& zPS,
	const Eigen::MatrixXf& zSfM,
	const Eigen::MatrixXf& mask, // 1.0 where both zPS and zSfM are valid
	double inlierThreshold = 0.01 // Adjust based on your depth unit scale
	) {
	int rows = zPS.rows();
	int cols = zPS.cols();

	// 1. Collect valid corresponding samples
	std::vector<SamplePoint> samples;
	for (int r = 0; r < rows; r += 4) { // Stride of 4 for speed
		for (int c = 0; c < cols; c += 4) {
			if (mask(r, c) == 1.0) {
				// Normalize coordinates to [-1, 1] for numerical stability in polynomial expansion
				double normX = (2.0 * c / cols) - 1.0;
				double normY = (2.0 * r / rows) - 1.0;
				samples.push_back({normX, normY, static_cast<double>(zPS(r, c)), static_cast<double>(zSfM(r, c))});
			}
		}
	}



	if (samples.size() < 10) return zPS; // Fallback if insufficient overlap

	

	// 2. RANSAC to solve for scale 'a' and polynomial warp coefficients
	std::mt19937 rng(1337);
	std::uniform_int_distribution<size_t> dist(0, samples.size() - 1);

	size_t bestInliers = 0;
	Eigen::VectorXd bestCoeffs = Eigen::VectorXd::Zero(7);
	std::vector<size_t> bestInlierIndices;

	for (int iter = 0; iter < 500; ++iter) {
		std::vector<size_t> idx;
		while (idx.size() < 7) {
			size_t rIdx = dist(rng);
			if (std::find(idx.begin(), idx.end(), rIdx) == idx.end()) idx.push_back(rIdx);
		}

		Eigen::VectorXd coeffs = fitWarpModel(samples, idx);

		std::vector<size_t> inliers;
		for (size_t i = 0; i < samples.size(); ++i) {
			const auto& s = samples[i];
			Eigen::Matrix<double, 1, 7> row;
			row << s.zPS, 1.0, s.x, s.y, s.x * s.x, s.x * s.y, s.y * s.y;
			double pred = row * coeffs;
			if (std::abs(pred - s.zSfM) < inlierThreshold) {
				inliers.push_back(i);
			}
		}

		if (inliers.size() > bestInliers) {
			bestInliers = inliers.size();
			bestCoeffs = coeffs;
			bestInlierIndices = inliers;
		}
	}

	// Refine coefficients on all inliers
	if (bestInliers >= 10) {
		bestCoeffs = fitWarpModel(samples, bestInlierIndices);
	}

	// 3. Apply continuous warp correction to full PS depth map
	Eigen::MatrixXf correctedPS = Eigen::MatrixXf::Zero(rows, cols);
	double a = bestCoeffs(0);

	cout << "a: " << a << endl;

	for (int r = 0; r < rows; ++r) {
		for (int c = 0; c < cols; ++c) {
			if (mask(r, c) > 0.5f) {
				double normX = (2.0 * c / cols) - 1.0;
				double normY = (2.0 * r / rows) - 1.0;

				double polyWarp = bestCoeffs(1) +
								  bestCoeffs(2) * normX +
								  bestCoeffs(3) * normY +
								  bestCoeffs(4) * normX * normX +
								  bestCoeffs(5) * normX * normY +
								  bestCoeffs(6) * normY * normY;

				correctedPS(r, c) = static_cast<float>(a * zPS(r, c) + polyWarp);
			}
		}
	}

	return correctedPS;
}

#include <Eigen/Dense>
#include <vector>
#include <cmath>
#include <iostream>

// Thin Plate Spline Basis Function: U(r) = r^2 * ln(r)
inline double tpsKernel(double r) {
	if (r <= 1e-8) return 0.0;
	return r * r * std::log(r);
}

// Computes TPS Warp Coefficients (W, A) given control points and residual target values
struct TPSModel2D {
	Eigen::VectorXd w; // Radial basis weights (N x 1)
	Eigen::Vector3d a; // Affine coefficients [a0, a_x, a_y]
	Eigen::MatrixXd controlPoints; // (N x 2) matrix of [x, y] coordinates
};

TPSModel2D fitTPSWarp(
	const std::vector<Eigen::Vector2d>& pts,
	const std::vector<double>& residuals,
	double lambda = 1e-2 // Regularization parameter controlling smoothness
	) {
	const size_t N = pts.size();
	TPSModel2D model;

	if (N < 3) {
		std::cerr << "Error: TPS requires at least 3 non-collinear control points.\n";
		return model;
	}

	model.controlPoints.resize(N, 2);
	for (size_t i = 0; i < N; ++i) {
		model.controlPoints.row(i) = pts[i];
	}

	// 1. Build K matrix (N x N) using TPS Radial Basis Function
	Eigen::MatrixXd K(N, N);
	for (size_t i = 0; i < N; ++i) {
		for (size_t j = i; j < N; ++j) {
			if (i == j) {
				K(i, j) = lambda; // Regularization on diagonal
			} else {
				double dist = (pts[i] - pts[j]).norm();
				double val = tpsKernel(dist);
				K(i, j) = val;
				K(j, i) = val;
			}
		}
	}

	// 2. Build P matrix (N x 3) for affine components [1, x, y]
	Eigen::MatrixXd P(N, 3);
	for (size_t i = 0; i < N; ++i) {
		P.row(i) << 1.0, pts[i].x(), pts[i].y();
	}

	// 3. Assemble full system matrix (N+3 x N+3):
	// [ K + lambda*I   P ] [ W ] = [ V ]
	// [     P^T        0 ] [ A ]   [ 0 ]
	Eigen::MatrixXd L = Eigen::MatrixXd::Zero(N + 3, N + 3);
	L.block(0, 0, N, N) = K;
	L.block(0, N, N, 3) = P;
	L.block(N, 0, 3, N) = P.transpose();

	Eigen::VectorXd Y = Eigen::VectorXd::Zero(N + 3);
	for (size_t i = 0; i < N; ++i) {
		Y(i) = residuals[i];
	}

	// 4. Solve symmetric system via SVD / ColPivHouseholderQR
	Eigen::VectorXd sol = L.colPivHouseholderQr().solve(Y);

	model.w = sol.head(N);
	model.a = sol.tail(3);

	return model;
}

// Evaluates Thin Plate Spline at a single 2D point (x, y)
double evaluateTPS(const TPSModel2D& model, double x, double y) {
	const size_t N = model.controlPoints.rows();
	Eigen::Vector2d pt(x, y);

	// Affine part: a0 + a_x * x + a_y * y
	double val = model.a(0) + model.a(1) * x + model.a(2) * y;

	// Non-linear radial basis part
	for (size_t i = 0; i < N; ++i) {
		double dist = (pt - model.controlPoints.row(i).transpose()).norm();
		val += model.w(i) * tpsKernel(dist);
	}

	return val;
}

// Main function: Corrects PS depthmap warp using TPS matching against SfM depth map
Eigen::MatrixXf correctPSDepthTPS(
	const Eigen::MatrixXf& zPS,  // Photometric Stereo Depth (Perspective/Ortho)
	const Eigen::MatrixXf& zSfM, // Structure-from-Motion Depth Scaffolding
	const Eigen::MatrixXf& mask, // 1.0 where both zPS and zSfM are valid
	int stride = 16,             // Control point sampling density (e.g., sample every 16th pixel)
	double lambda = 1e-2         // TPS bending stiffness (higher = smoother warp)
	) {
	const int rows = zPS.rows();
	const int cols = zPS.cols();

	std::vector<Eigen::Vector2d> controlPoints;
	std::vector<double> residuals;

	// 1. Collect control points where both depth maps are valid
	for (int r = stride; r < rows-stride; r += stride) {
		for (int c = stride; c < cols-stride; c += stride) {
			float psVal = zPS(r, c);
			float sfmVal = zSfM(r, c);
			float m = mask(r, c);
			// Valid depth check (non-zero and non-NaN)
			if (m == 1.0f) {
				// Normalize pixel coordinates to [-1, 1] for numerical stability
				double normX = (2.0 * c / cols) - 1.0;
				double normY = (2.0 * r / rows) - 1.0;

				controlPoints.push_back({normX, normY});
				residuals.push_back(static_cast<double>(sfmVal - psVal));
			}
		}
	}
	cout << "Control points size: " << controlPoints.size() << endl;
	if (controlPoints.size() < 3) {
		std::cerr << "Warning: Insufficient overlapping points for TPS fitting. Returning raw PS depth.\n";
		return zPS;
	}

	// 2. Fit Thin Plate Spline deformation surface
	TPSModel2D tpsModel = fitTPSWarp(controlPoints, residuals, lambda);

	// 3. Apply smooth TPS warp correction to every valid pixel in PS depthmap
	Eigen::MatrixXf correctedPS = zPS;

	for (int r = 0; r < rows; ++r) {
		for (int c = 0; c < cols; ++c) {
			float psVal = zPS(r, c);
			float sfmVal = zSfM(r, c);
			float m = mask(r, c);

			double normX = (2.0 * c / cols) - 1.0;
			double normY = (2.0 * r / rows) - 1.0;

			double warpOffset = evaluateTPS(tpsModel, normX, normY);
//			correctedPS(r, c) = sfmVal*m + (1-m)*(psVal + static_cast<float>(warpOffset));
			correctedPS(r, c) = psVal + static_cast<float>(warpOffset);

		}
	}

	return correctedPS;
}

void OrthoDepthmap::anchorCamera(const CameraDepthmap& camera, const char *outputFile){
	auto &cam = camera.camera;

	// Project ortho depth onto camera image, storing directly in a Depthmap
	Depthmap projected;
	int depthSize = cam.width * cam.height;
	projected.elevation.resize(depthSize, 0.0f);
	projected.mask.resize(depthSize, 0.0f);
	std::vector<float> proj_correlation(depthSize, 0.0f);

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if(correlation[x + y * width] == 0.0f)
				continue;
			float pixelZ = old_elevation[x + y * width];

			Eigen::Vector3f realCoordinates = pixelToRealCoordinates(x, y, pixelZ);
			Eigen::Vector3f imageCoords = cam.projectionToImage(realCoordinates);

			int imageX = (int)round(imageCoords[0]);
			int imageY = (int)round(imageCoords[1]);

			if (imageX >= 0 && imageX < cam.width && imageY >= 0 && imageY < cam.height) {
				projected.elevation[imageX + imageY * cam.width] = imageCoords[2];
				projected.mask[imageX + imageY * cam.width] = 1.0f;
				proj_correlation[imageX + imageY * cam.width] = correlation[x + y * width];
			}
		}
	}



	fillDepthMapHoles(projected.elevation, proj_correlation, projected.mask, cam.width, cam.height);
	static int c = 0;

	Depthmap::saveTiff(QString("ortho_correlation_%1.tif").arg(c).toStdString().c_str(), proj_correlation, cam.width, cam.height, 32);



	// Fuse camera depth (high detail) with projected ortho (structure)
	// zPS_pixels is expressed relative to camera pixels (see scale computation below)
	Eigen::MatrixXf zPS_pixels = Eigen::MatrixXf::Zero(cam.height, cam.width);
	for (int y = 0; y < cam.height; y++) {
		for (int x = 0; x < cam.width; x++) {
			zPS_pixels(y, x) = camera.elevation[x + y * cam.width];
		}
	}

	Eigen::MatrixXf zSfM = Eigen::MatrixXf::Zero(cam.height, cam.width);
	for (int y = 0; y < cam.height; y++) {
		for (int x = 0; x < cam.width; x++) {
			zSfM(y, x) = projected.elevation[x + y * cam.width];
		}
	}
	Eigen::MatrixXf zMask = Eigen::MatrixXf::Zero(cam.height, cam.width);
	for (int y = 0; y < cam.height; y++) {
		for (int x = 0; x < cam.width; x++) {
			zMask(y, x) = proj_correlation[x + y * cam.width] > 0? 1 : 0;
		}
	}

	// camera.elevation is expressed relative to camera pixels (a step of 1 pixel in x/y),
	// while projected.elevation (zSfM) is expressed in camera coordinates (real units along
	// the camera Z axis, see Camera::projectionToImage). Since a pixel's real-world size is
	// z/focal (bigger the further away it is), use the average valid projected Z as an
	// approximation of the real depth to convert camera.elevation into the same units as zSfM.
	double sumZ = 0.0;
	size_t countZ = 0;
	for (int y = 0; y < cam.height; y++) {
		for (int x = 0; x < cam.width; x++) {
			if (projected.mask[x + y * cam.width] > 0.0f) {
				sumZ += projected.elevation[x + y * cam.width];
				++countZ;
			}
		}
	}
	float avgZ = countZ > 0 ? static_cast<float>(sumZ / double(countZ)) : 0.0f;
	float pixelScale = -((cam.focal > 1e-6f) ? (avgZ / cam.focal) : 1.0f);
	cout << "Average projected Z: " << avgZ << ", focal: " << cam.focal << ", pixelScale: " << pixelScale << endl;

	Eigen::MatrixXf zPS = zPS_pixels * pixelScale;

	Eigen::MatrixXf fused;

	bool using_frequencies = false;

	if(using_frequencies) {
		// Fill holes using the helper


		fused = fuseDepthFrequencySplit(zPS, zSfM, 10.0f);
	}

	bool using_pol = false;
	if(using_pol) {
		float threshold = 1;
		fused = correctPSWithPolynomialWarp(zPS, zSfM, zMask, threshold);
	}

	bool using_thinplate = true;
	if(using_thinplate) {
		//fused = zSfM;
		tps_grid_step = 50;
		fused = correctPSDepthTPS(zPS, zSfM, zMask, tps_grid_step);
	}
	std::vector<float> orthoDepth(width*height, 0.0f);

	for (int oy = 0; oy < height; oy++) {
		for (int ox = 0; ox < width; ox++) {
			int oidx = ox + oy * width;
			// use old ortho elevation as starting real Z
			float orthoZ = old_elevation[oidx];
			Eigen::Vector3f realCoord = pixelToRealCoordinates(ox, oy, orthoZ);
			Eigen::Vector3f imgCoords = cam.projectionToImage(realCoord);
			int ix = (int)std::round(imgCoords[0]);
			int iy = (int)std::round(imgCoords[1]);
			if (ix < 0 || ix >= cam.width || iy < 0 || iy >= cam.height) continue;

			// read fused depth at camera pixel
			float fusedDepth = fused(iy, ix);

			// back-project camera pixel+fusedDepth to real coords
			Eigen::Vector3f realFromCam = cam.projectionToReal(Eigen::Vector3f(ix, iy, fusedDepth));
			Eigen::Vector3f orthoPixel = realToPixelCoord(realFromCam[0], realFromCam[1], realFromCam[2]);
			float h = orthoPixel[2];
			orthoDepth[ox + width*oy] = h;

			float w = camera.calculateWeight(ix, iy);
			elevation[oidx] += w * h;
			weights[oidx] += w;
		}
	}
	cout << endl;
	Depthmap::saveTiff(QString("ortho_projection_%1.tif").arg(c++).toStdString().c_str(), orthoDepth, width, height, 32);
}

void OrthoDepthmap::integratedCamera(const CameraDepthmap& camera, const char *outputFile){

	QFile outFile(outputFile);
	if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		cerr << "Errore nell'aprire il file di output: " << outputFile << endl;
		return;
	}

	QTextStream out(&outFile);
	std::vector<Eigen::Vector3f> imageCloud;
	std::vector<float> source;

	out << "x\ty\tz\n";
	for (size_t i = 0; i < point_cloud.size(); i++) {

		Eigen::Vector3f realCoord = point_cloud[i];
		float h = realCoord[2];
		//Eigen::Vector3f pixelCoord = realToPixelCoord(realCoord[0], realCoord[1], realCoord[2]);
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

	gaussianGrid.minSamples = 10;//TODO: 3 seems a good value // 1, 3, 5
	gaussianGrid.sideFactor = 0.01; //0.25; //0.125; // 0.25, 0.5, 1, 2, 0.125
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
