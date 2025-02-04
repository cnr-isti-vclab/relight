#include "imagealignment.h"

#include <iostream>
using namespace std;

void ImageAlignment::alignSamples(bool useECC) {
	if (samples.empty()) return;

	const cv::Mat& ref = samples[0];
	offsets.resize(samples.size(), cv::Point2f(0, 0));

	for (size_t i = 1; i < samples.size(); i++) {
		try {
			cv::Mat warpMat = useECC ? computeECC(samples[i], ref) : computeMutualInformation(samples[i], ref);

			offsets[i] = cv::Point2f(warpMat.at<float>(0, 2), warpMat.at<float>(1, 2));
		} catch(cv::Exception &e) {
			cerr << e.msg << endl;
			offsets[i] = cv::Point2f(0.0f, 0.0f);
		}
	}
}


double mutualInformation(const cv::Mat& a, const cv::Mat& b) {
	std::vector<int> histo(256*256, 0);
	std::vector<int> aprob(256, 0);
	std::vector<int> bprob(256, 0);
	int width = a.cols;
	int height = a.rows;
	//x and y refer to the image pixels!
	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			int ca = a.at<unsigned char>(y, x);
			int cb = b.at<unsigned char>(y, x);
			histo[ca + 256*cb]++;
			aprob[ca]++;
			bprob[cb]++;
		}
	}
	// w * h vettore, trova min e max, scala tra 0 e 1 e dopo lo metti su una qimg
	//
	double tot = height*width;
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


void ImageAlignment::testAlign() {
	int k = 10;
	int w = samples[0].cols;
	int h = samples[0].rows;
	cv::Mat a = samples[0](cv::Rect(k, k, w-2*k, h-2*k));
	for(int i = 0; i < samples.size(); i++) {
		cv::Mat result(2*k+1, 2*k+1, CV_32F);
		for(int dy = -k; dy <= k; dy++) {
			for(int dx = -k; dx <= k; dx++) {
				cv::Mat b = samples[i](cv::Rect(k + dx, k+dy, w-2*k, h-2*k));
				float my = mutualInformation(a, b);
				//float my = computeECCValue(a, b);
				result.at<float>(dy +k, dx+k) = my;
			}
		}
		cv::Mat normalizedMat, uint8Mat;
		cv::normalize(result, normalizedMat, 0, 255, cv::NORM_MINMAX);
		normalizedMat.convertTo(uint8Mat, CV_8U);
		cv::imwrite(std::to_string(i) + ".png", uint8Mat);
	}
}


double ImageAlignment::computeECCValue(const cv::Mat& src, const cv::Mat& ref) {
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

cv::Mat ImageAlignment::computeECC(const cv::Mat& src, const cv::Mat& ref) {
	cv::Mat warpMat = cv::Mat::eye(2, 3, CV_32F);
	cv::findTransformECC(ref, src, warpMat, cv::MOTION_TRANSLATION);
	return warpMat;
}

double ImageAlignment::computeMutualInformationValue(const cv::Mat& src, const cv::Mat& ref) {
	return mutualInformation(src, ref);
	const int histSize = 256;
	float range[] = { 0, 256 };
	const float* histRange = { range };

	cv::Mat histSrc, histRef, jointHist;
	int channels[] = {0};
	cv::calcHist(&src, 1, channels, cv::Mat(), histSrc, 1, &histSize, &histRange);
	cv::calcHist(&ref, 1, channels, cv::Mat(), histRef, 1, &histSize, &histRange);
	cv::calcHist(&src, 1, channels, ref, jointHist, 2, &histSize, &histRange);

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

cv::Mat ImageAlignment::computeMutualInformation(const cv::Mat& src, const cv::Mat& ref) {
	cv::Mat warpMat = cv::Mat::eye(2, 3, CV_32F);

	cv::Matx21f delta(0, 0);
	double prevMI = computeMutualInformationValue(src, ref);

	for (int iter = 0; iter < 20; iter++) {
		cv::Mat warpedSrc;
		cv::warpAffine(src, warpedSrc, warpMat, ref.size());

		double newMI = computeMutualInformationValue(warpedSrc, ref);
		if (newMI <= prevMI) break;
		prevMI = newMI;

		cv::Matx21f grad;
		grad(0) = (computeMutualInformationValue(warpedSrc(cv::Rect(1, 0, warpedSrc.cols - 1, warpedSrc.rows)), ref) - newMI);
		grad(1) = (computeMutualInformationValue(warpedSrc(cv::Rect(0, 1, warpedSrc.cols, warpedSrc.rows - 1)), ref) - newMI);

		cv::solve(cv::Matx22f(1, 0, 0, 1), grad, delta, cv::DECOMP_SVD);

		warpMat.at<float>(0, 2) += delta(0);
		warpMat.at<float>(1, 2) += delta(1);
	}
	return warpMat;
}
