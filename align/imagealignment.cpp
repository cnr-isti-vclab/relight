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
