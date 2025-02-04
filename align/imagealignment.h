#ifndef IMAGEALIGNMENT_H
#define IMAGEALIGNMENT_H


#include <opencv2/opencv.hpp>
#include <vector>
#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>

class ImageAlignment {
public:
	cv::Rect2f region;
	std::vector<cv::Mat> samples;
	std::vector<cv::Point2f> offsets;

	ImageAlignment(const cv::Rect2f& region_): region(region_) {}

	void alignSamples(bool useECC = true);
	void testAlign();

private:
	double computeECCValue(const cv::Mat& src, const cv::Mat& ref);
	cv::Mat computeECC(const cv::Mat& src, const cv::Mat& ref);

	double computeMutualInformationValue(const cv::Mat& src, const cv::Mat& ref);
	cv::Mat computeMutualInformation(const cv::Mat& src, const cv::Mat& ref);
};
#endif // IMAGEALIGNMENT_H
