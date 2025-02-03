#include <math.h>

#include <iostream>
#include <vector>

#include <QImage>
#include <QPoint>
#include <QDir>
#include <QTextStream>

#include <assert.h>
//#include <QGuiApplication>

//#include "aligndialog.h"

using namespace std;

/* Align: fine alignmend of accidentally slightly misaligned images during capture.
 *
 * max number of pixl in input
 * mode: initially only translate is supported, later probably add rotate
 * test run (wont convert the images, just return translations)
 * Final images are cropped to the common portion
 * Result should be inspectable in an interface.
 *
 * Mutual information is used for alignment on a few selected region.
 * can be specified, or auto (defaults).
 */

//a is the x axis (col) b is the y axs (the col)
double mutualInformation(QImage a, QImage b, int max, int dx, int dy) {
	std::vector<int> histo(256*256, 0);
	std::vector<int> aprob(256, 0);
	std::vector<int> bprob(256, 0);
	int width = a.width();
	int height = a.height();
	//x and y refer to the image pixels!
	for(int y = max; y < height - max; y++) {
		for(int x = max; x < width - max; x++) {
			QRgb ca = qGray(a.pixel(x, y));
			QRgb cb = qGray(b.pixel(x + dx, y + dy));
			histo[ca + 256*cb]++;
			aprob[ca]++;
			bprob[cb]++;
		}
	}
	// w * h vettore, trova min e max, scala tra 0 e 1 e dopo lo metti su una qimg
	//
	double tot = (height - 2*max)*(width - 2*max);
	double info = 0.0;

	for(int y = 0; y < 256; y++) {
		for(int x = 0; x < 256; x++) {
			double p = histo[x + 256*y]/tot;
			if(p == 0) continue;
			double pa = aprob[x]/tot;
			double pb = bprob[y]/tot;
			info += p * log(p/(pa*pb));
		}
	}
	return info;
}




//a and b image must be decently larger than max min is 3 timess
//bw images
QPoint align(QImage a, QImage b, int max, double &best_info, double &initial) {

	int max_side = max;
	best_info = 0.0;
	double worst_info = 1e20;
	QPoint best(0, 0);
	int side = 2*max_side + 1;
	std::vector<double> values(side*side);
	for(int dy = -max_side; dy <= max_side; dy++) {
		for(int dx = -max_side; dx <= max_side; dx++) {
			double info = mutualInformation(a, b, max, dx, dy);
			if(dx == 0 && dy == 0) {
				initial = info;
			}
			assert(dx+max_side + (dy+max_side)*side < values.size());
			values[dx+max_side + (dy+max_side)*side] = info;
			worst_info = std::min(worst_info, info);
			//cout << info << " ";
			if(info > best_info) {
				best_info = info;
				best.rx() = dx;
				best.ry() = dy;
			}
		}
		//cout << endl;
	}
	QImage img(side, side, QImage::Format::Format_RGB888);
	for(int y = 0; y < side; y++) {
		for(int x = 0; x < side; x++) {
			double g = 255*(values[x + y*side] - worst_info)/(best_info - worst_info);
			img.setPixel(x, y, qRgb(g, g, g));
		}
	}
	static int count = 0;
	img.save(QString("mutual%1.png").arg(count++));
	return best;
}

struct Offset {
	int x;
	int y;
};

std::vector<Offset> readOffsetsCSV(const QString &filePath) {
	std::vector<Offset> offsets;
	QFile file(filePath);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		cerr << "Could not open: " << qPrintable(filePath) << endl;
		return offsets;
	}

	QTextStream in(&file);
	bool firstLine = true;
	while (!in.atEnd()) {
		QString line = in.readLine().trimmed();
		if (firstLine) {
			firstLine = false;
			continue;
		}
		QStringList values = line.split(",");
		if (values.size() >= 2) {
			offsets.push_back({values[0].trimmed().toInt(), values[1].trimmed().toInt()});
		}
	}
	file.close();
	return offsets;
}

void analyzeErrors(const std::vector<QPoint> &calculatedOffsets, const std::vector<Offset> &trueOffsets) {
	if (calculatedOffsets.size() != trueOffsets.size()) {
		cerr << "Number of compute offsets and real offsets not matching" << endl;
		return;
	}
	double sumSqErrorX = 0, sumSqErrorY = 0;
	int countPixel1 = 0, countPixel2 = 0, countPixel3 = 0;
	double minErrorX = std::numeric_limits<double>::max();
	double maxErrorX = std::numeric_limits<double>::min();
	double minErrorY = std::numeric_limits<double>::max();
	double maxErrorY = std::numeric_limits<double>::min();

	cout << "Differenze calcolate:" << endl;
	for (size_t i = 0; i < calculatedOffsets.size(); i++) {
		int dx = std::abs(calculatedOffsets[i].x() - trueOffsets[i].x);
		int dy = std::abs(calculatedOffsets[i].y() - trueOffsets[i].y);
		double error = std::sqrt(dx * dx + dy * dy);  // Distanza euclidea

		cout << "Offset [" << i << "] - Calcolato: (" << calculatedOffsets[i].x() << ", " << calculatedOffsets[i].y()
			 << "), Reale: (" << trueOffsets[i].x << ", " << trueOffsets[i].y << "), ΔX: " << dx << ", ΔY: " << dy
			 << ", Errore: " << error << endl;

		sumSqErrorX += dx * dx;
		sumSqErrorY += dy * dy;

		minErrorX = std::min(minErrorX, (double)dx);
		maxErrorX = std::max(maxErrorX, (double)dx);
		minErrorY = std::min(minErrorY, (double)dy);
		maxErrorY = std::max(maxErrorY, (double)dy);

		if (error >= 1) countPixel1++;
		if (error >= 2) countPixel2++;
		if (error >= 3) countPixel3++;


	}
	/*
		if (dx >= 1 || dy >= 1) countPixel1++;
		if (dx >= 2 || dy >= 2) countPixel2++;
		if (dx >= 3 || dy >= 3) countPixel3++;
	}*/


	double rmseX = std::sqrt(sumSqErrorX / calculatedOffsets.size());
	double rmseY = std::sqrt(sumSqErrorY / calculatedOffsets.size());
	cout << "\nStatistiche Errori:" << endl;
	cout << "Errore quadratico medio (RMSE) X: " << rmseX << ", Y: " << rmseY << endl;
	cout << "Errore minimo X: " << minErrorX << ", Y: " << minErrorY << endl;
	cout << "Errore massimo X: " << maxErrorX << ", Y: " << maxErrorY << endl;
	cout << "Offset con errore ≥ 1 pixel: " << countPixel1 << " (" << (countPixel1 * 100.0) / calculatedOffsets.size() << "%)" << endl;
	cout << "Offset con errore ≥ 2 pixel: " << countPixel2 << " (" << (countPixel2 * 100.0) / calculatedOffsets.size() << "%)" << endl;
	cout << "Offset con errore ≥ 3 pixel: " << countPixel3 << " (" << (countPixel3 * 100.0) / calculatedOffsets.size() << "%)" << endl;
}

#include <opencv4/opencv2/opencv.hpp>
#include <vector>
#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>

class ImageAlignment {
public:
	cv::Rect2f region;  // Defined rectangular region
	std::vector<cv::Mat> samples; // Small images
	std::vector<cv::Point2f> offsets; // Translation offsets

	// Constructor
	ImageAlignment(const cv::Rect2f& region_): region(region_) {}

	// Align all samples to the first using ECC or Mutual Information
	void alignSamples(bool useECC = true) {
		if (samples.empty()) return;

		const cv::Mat& ref = samples[0];
		offsets.resize(samples.size(), cv::Point2f(0, 0));

		for (size_t i = 1; i < samples.size(); i++) {
			try {
				cv::Mat warpMat = useECC ? computeECC(samples[i], ref) : computeMutualInformation(samples[i], ref);

				offsets[i] = cv::Point2f(warpMat.at<float>(0, 2), warpMat.at<float>(1, 2));
			} catch(cv::Exception e) {
				offsets[i] = cv::Point2f(-1000, -1000);
			}
		}
	}

private:
	double computeECCValue(const cv::Mat& src, const cv::Mat& ref) {
		   cv::Mat srcFloat, refFloat;
		   src.convertTo(srcFloat, CV_32F);
		   ref.convertTo(refFloat, CV_32F);

		   cv::Mat srcSquared, refSquared, srcRefMult;
		   cv::multiply(srcFloat, srcFloat, srcSquared);
		   cv::multiply(refFloat, refFloat, refSquared);
		   cv::multiply(srcFloat, refFloat, srcRefMult);

		   double num = cv::sum(srcRefMult)[0];
		   double denom = std::sqrt(cv::sum(srcSquared)[0] * cv::sum(refSquared)[0]);

		   return (denom > 0) ? num / denom : 0.0;
	   }

	// Alignment methods
	cv::Mat computeECC(const cv::Mat& src, const cv::Mat& ref) {
		cv::Mat warpMat = cv::Mat::eye(2, 3, CV_32F);
		cv::findTransformECC(ref, src, warpMat, cv::MOTION_TRANSLATION);
		return warpMat;
	}

	double computeMutualInformationValue(const cv::Mat& src, const cv::Mat& ref) {
		const int histSize = 256;
		float range[] = { 0, 256 };
		const float* histRange = { range };

		cv::Mat histSrc, histRef, jointHist;
		cv::calcHist(&src, 1, 0, cv::Mat(), histSrc, 1, &histSize, &histRange);
		cv::calcHist(&ref, 1, 0, cv::Mat(), histRef, 1, &histSize, &histRange);
		cv::calcHist(&src, 1, 0, ref, jointHist, 2, &histSize, &histRange);

		cv::normalize(histSrc, histSrc, 1, 0, cv::NORM_L1);
		cv::normalize(histRef, histRef, 1, 0, cv::NORM_L1);
		cv::normalize(jointHist, jointHist, 1, 0, cv::NORM_L1);

		double Hs = 0, Hr = 0, Hsr = 0;
		for (int i = 0; i < histSize; i++) {
			float ps = histSrc.at<float>(i);
			float pr = histRef.at<float>(i);
			if (ps > 0) Hs -= ps * std::log2(ps);
			if (pr > 0) Hr -= pr * std::log2(pr);
		}
		for (int i = 0; i < histSize; i++) {
			for (int j = 0; j < histSize; j++) {
				float psr = jointHist.at<float>(i, j);
				if (psr > 0) Hsr -= psr * std::log2(psr);
			}
		}

		return Hs + Hr - Hsr;
	}

	cv::Mat computeMutualInformation(const cv::Mat& src, const cv::Mat& ref) {
		cv::Mat warpMat = cv::Mat::eye(2, 3, CV_32F);

		cv::Matx21f delta(0, 0);
		double prevMI = computeMutualInformationValue(src, ref);

		for (int iter = 0; iter < 20; iter++) {
			cv::Mat warpedSrc;
			cv::warpAffine(src, warpedSrc, warpMat, ref.size());

			double newMI = computeMutualInformationValue(warpedSrc, ref);
			if (newMI <= prevMI) break;
			prevMI = newMI;

			// Compute gradient approximation
			cv::Matx21f grad;
			grad(0) = (computeMutualInformationValue(warpedSrc(cv::Rect(1, 0, warpedSrc.cols - 1, warpedSrc.rows)), ref) - newMI);
			grad(1) = (computeMutualInformationValue(warpedSrc(cv::Rect(0, 1, warpedSrc.cols, warpedSrc.rows - 1)), ref) - newMI);

			// Use OpenCV's solve function to refine translation parameters
			cv::solve(cv::Matx22f(1, 0, 0, 1), grad, delta, cv::DECOMP_SVD);

			warpMat.at<float>(0, 2) += delta(0);
			warpMat.at<float>(1, 2) += delta(1);
		}
		return warpMat;
	}
};




#include "../src/getopt.h"
#include <QImage>
#include <QRandomGenerator>

int main(int argc, char *argv[]) {

	if(argc == 1) {
		/*		QGuiApplication app(argc, argv);
		auto dialog = new AlignDialog;
		dialog->show();
		int res = dialog->exec();
		return res; */
	}

	//dir max_offset crop
	if(argc != 4) {
		cerr << "Usage: " << argv[0] << " directory max_offset top:left:width:height" << endl;
		return 0;
	}

	QDir dir = QDir(argv[1]);
	if(!dir.exists()) {
		cerr << "Could not find " << qPrintable(argv[1]) << " folder.\n";
		return -1;
	}
	QStringList img_ext;
	img_ext << "*.jpg" << "*.JPG";;
	QStringList images = dir.entryList(img_ext);
	if(!images.size()) {
		cerr << "No images to align!" << endl;
		return -1;
	}
	QImage first(dir.filePath(images[0]));
	if(first.isNull()) {
		cerr << "Could not load image: " << qPrintable(images[0]) << endl;
		return -1;
	}

	int max_offset = QString(argv[2]).toInt();
	int radius = max_offset / 4;
	QStringList c = QString(argv[3]).split(":");

	//random
	QRandomGenerator *generator = QRandomGenerator::global();

	QRect crop(c[0].toInt(), c[1].toInt(), c[2].toInt(), c[3].toInt());
	crop.adjust(-max_offset, -max_offset, max_offset, max_offset);

	if(!QRect(0, 0, first.width(), first.height()).contains(crop)) {
		cerr << "Alignment sample (+offset) is not contained in the image" << endl;
		return -1;
	}
	cv::Rect2f rect(crop.left(), crop.top(), crop.width(), crop.height());
	ImageAlignment alignment(rect);
	vector<cv::Point2f> original_offsets;
	vector<cv::Mat> &samples = alignment.samples;
	for(int i = 0; i < images.size(); i++) {
		cout << qPrintable(images[i]) << endl;
		cv::Mat img = cv::imread(images[i].toStdString(), cv::IMREAD_GRAYSCALE);
		int dx = generator->bounded(-radius, radius);
		int dy = generator->bounded(-radius, radius);
		if(i == 0) {
			dx = dy = 0;
		}
		cv::Rect2f region(crop.left() + dx, crop.top() + dy, crop.width(), crop.height());

		samples.push_back(img(region));
		original_offsets.push_back(cv::Point2f(-dx, -dy));
	}

	alignment.alignSamples(true);
	for(int i = 0; i < original_offsets.size(); i++) {
		cout << alignment.offsets[i].x - original_offsets[i].x << "       " << alignment.offsets[i].y - original_offsets[i].y << endl;
	}

	/*
	int reference = 0;
	vector<QPoint> original_offsets;
	QPoint origin(0,0);
	for(int i = 0; i < samples.size(); i++) {
		double best = 0.0f;
		double initial = 0.0f;
		QPoint p = align(samples[reference], samples[i], max_offset, best, initial);
		cout << p.x() << " " << p.y() << ": " << best << endl;
		original_offsets.push_back(p);
	}

	QString offsetsFile = dir.filePath("offsets.csv");
	vector<Offset> trueOffsets = readOffsetsCSV(offsetsFile);

	analyzeErrors(offsets, trueOffsets); */

	//1. cerca file offsets.csv, split per " ", confronti i due array e confronti la differenza.
	// 2. calcola differenza e percentuale >= 1 pixel in x e y, quante volte arrivi al pixel 1, 2, 3 ecc.
	// 3. errore quadr medio, errore min, err mass, elenco errori

	/*
	QDir dir("./");
	dir.setNameFilters(QStringList()<<"*.jpg"<<"*.JPG");
	QStringList images = dir.entryList();

	int reference = 0;
	int max = 20;
	int width = 190; //40;
	int height = 190; //40;
	int offx = 3017;//51; //100;
	int offy = 3837;//157; //100;
	vector<QImage> samples;
	for(int i = 0; i < images.size(); i++) {
		cout << qPrintable(images[i]) << endl;
		QImage img(images[i]);
		QImage sub = img.copy(offx, offy, width, height);
		samples.push_back(sub);
	}

	vector<QPoint> offsets;
	for(int i = 0; i < samples.size(); i++) {
		if(i == reference) {
			offsets.push_back(QPoint(0, 0));
			continue;
		}
		QPoint p = align(samples[reference], samples[i], max);
		cout << p.x() << " " << p.y() << endl << endl;
		offsets.push_back(p);
	}
	dir.mkdir("aligned");
	for(int i = 0; i < images.size(); i++) {
		QPoint &o = offsets[i];
	//	QImage img(images[i]);
		QImage img = samples[i];
		QImage tmp = img.copy(max + o.x(), max + o.y(), img.width() - 2*max, img.height() - 2*max);
		tmp.save("aligned/A_" + images[i]);
	} */

	return 0;
}

