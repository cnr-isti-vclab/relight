#include "ortholoader.h"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/stitching/detail/blenders.hpp>
#include <opencv2/stitching/detail/seam_finders.hpp>

#include <QFileInfo>

#include <iostream>

using namespace std;

QImage bgrMatToQImage(const cv::Mat &bgr);
cv::Mat buildCoverageMask(const cv::Mat &source, const cv::Mat &loadedMask = cv::Mat());


int main(int argc, char *argv[]) {
	QString folder;
	QString outputPath;
	int numBands = 14;

	for (int i = 1; i < argc; ++i) {
		QString arg = QString::fromUtf8(argv[i]);
		if (arg == "-h" || arg == "--help") {
			cerr << "Usage: " << argv[0] << " [options] <input_folder> <output.png>" << endl;
			cerr << "Options:" << endl;
			cerr << "  -h, --help             Show this help message" << endl;
			cerr << "  -b, --bands <num>      Number of bands for MultiBandBlender (default: 14)" << endl;
			cerr << "  -f, --feather <radius> Feathering radius in pixels (default: 512.0)" << endl;
			cerr << endl;
			cerr << "Arguments:" << endl;
			cerr << "  <input_folder>         Folder containing TIFF files and TFW world files" << endl;
			cerr << "  <output.png>           Output image path" << endl;
			return 0;
		} else if (arg == "-b" || arg == "--bands") {
			if (++i < argc) {
				bool ok = false;
				numBands = QString::fromUtf8(argv[i]).toInt(&ok);
				if (!ok || numBands < 0 || numBands > 50) {
					cerr << "Invalid num_bands value. Must be between 0 and 50." << endl;
					return 1;
				}
			} else {
				cerr << "Missing value for " << qPrintable(arg) << endl;
				return 1;
			}
		} else if (!arg.startsWith("-")) {
			if (folder.isEmpty()) {
				folder = arg;
			} else if (outputPath.isEmpty()) {
				outputPath = arg;
			} else {
				cerr << "Unexpected argument: " << qPrintable(arg) << endl;
				return 1;
			}
		} else {
			cerr << "Unknown option: " << qPrintable(arg) << endl;
			return 1;
		}
	}

	if (folder.isEmpty() || outputPath.isEmpty()) {
		cerr << "Usage: " << argv[0] << " [options] <input_folder> <output.png>" << endl;
		cerr << "Try '" << argv[0] << " --help' for more information." << endl;
		return 1;
	}



	OrthoLoader loader;

	QString errorMessage;
	if(!loader.loadFromDirectory(folder, &errorMessage)) {
		cerr << "Loading failed: " << qPrintable(errorMessage) << endl;
		return 1;
	}

	const QVector<OrthoLoader::Tile> &tiles = loader.tiles();
	if (tiles.size() < 2) {
		cerr << "Not enough tiles: Need at least two tiles to blend." << endl;
		return 1;
	}

	const QSize canvasSize = loader.canvasSize();
	if (!canvasSize.isValid() || canvasSize.isEmpty()) {
		cerr << "Invalid canvas: Cannot blend because the canvas size is invalid." << endl;
		return 1;
	}

	// First pass: collect all images, masks, and tile info
	std::vector<cv::UMat> images_for_seam;
	std::vector<cv::Mat> images_bgr;
	std::vector<cv::Point> corners;
	std::vector<cv::UMat> masks;
	std::vector<cv::Size> sizes;
	std::vector<QString> tileNames;

	for (const OrthoLoader::Tile &tile : tiles) {
		// Load image with OpenCV
		cv::Mat bgr = cv::imread(tile.imagePath.toStdString(), cv::IMREAD_COLOR);
		if (bgr.empty())
			continue;

		// Load PC_ mask if availabl
		cv::Mat pcMask;
		if (!tile.maskPath.isEmpty() && QFileInfo::exists(tile.maskPath)) {
			pcMask = cv::imread(tile.maskPath.toStdString(), cv::IMREAD_GRAYSCALE);
		}

		cv::Mat mask = buildCoverageMask(bgr, pcMask);
		if (mask.empty() || cv::countNonZero(mask) == 0)
			continue;

		// Fill pixels outside mask with average color
		cv::Scalar avgColor = cv::mean(bgr, mask);
		cv::Mat zeroMask = (mask == 0);
		bgr.setTo(avgColor, zeroMask);

		// Convert to float for seam finding
		cv::Mat bgr_f;
		bgr.convertTo(bgr_f, CV_32F);

		images_for_seam.push_back(bgr_f.getUMat(cv::ACCESS_READ));
		images_bgr.push_back(bgr);
		masks.push_back(mask.getUMat(cv::ACCESS_RW));
		sizes.push_back(bgr.size());
		corners.push_back(cv::Point(tile.x, tile.y));
		tileNames.push_back(tile.name);
	}

	// Find seams to make masks non-overlapping
//	cv::Ptr<cv::detail::SeamFinder> seam_finder =
//		cv::makePtr<cv::detail::GraphCutSeamFinder>(cv::detail::GraphCutSeamFinder::COST_COLOR);
	cv::detail::VoronoiSeamFinder seam_finder;
	seam_finder.find(sizes, corners, masks);

	const cv::Rect roi(0, 0, canvasSize.width(), canvasSize.height());
	cv::detail::MultiBandBlender blender(false, numBands);
	blender.prepare(roi);

	// Second pass: feed to blender with modified masks
	for (size_t i = 0; i < loader.tiles().size(); ++i) {
		cv::Mat mask_cpu = masks[i].getMat(cv::ACCESS_READ);

		const OrthoLoader::Tile &tile = loader.tiles()[i];
		cv::Mat bgr = cv::imread(tile.imagePath.toStdString(), cv::IMREAD_COLOR);
		if (bgr.empty())
			continue;

		cv::Scalar avgColor = {0, 0, 0};// cv::mean(bgr, mask_cpu);
		cv::Mat zeroMask = (mask_cpu == 0);
//		bgr.setTo(avgColor, zeroMask);

//		cv::imwrite(tileNames[i].toStdString() + "b.png", bgr);
//		cv::imwrite(tileNames[i].toStdString() + "a.png", images_bgr[i]);
//		exit(0);
		cv::Mat img16s;
		//images_bgr[i].convertTo(img16s, CV_16SC3);
		bgr.convertTo(img16s, CV_16SC3);
		blender.feed(img16s, mask_cpu, corners[i]);
	}

	cv::Mat blended, blendedMask;
	blender.blend(blended, blendedMask);
	if (blended.empty()) {
		cerr << "Blending failed: OpenCV returned an empty result." << endl;
		return 1;
	}

	//cv::imwrite("blendednmask.png", blendedMask);

	cv::Mat blended8u;
	blended.convertTo(blended8u, CV_8UC3);
	QImage blendedImage = bgrMatToQImage(blended8u);
	if (blendedImage.isNull()) {
		cerr << "Blending failed: Could not convert the blended image for display." << endl;
		return 1;
	}

	// Save the output image
	if (!blendedImage.save(outputPath)) {
		cerr << "Failed to save output image to: " << qPrintable(outputPath) << endl;
		return 1;
	}
	return 0;
}


QImage bgrMatToQImage(const cv::Mat &bgr) {
	if (bgr.empty() || bgr.type() != CV_8UC3)
		return QImage();

	cv::Mat rgb;
	cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
	QImage image(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
	return image.copy();
}

cv::Mat buildCoverageMask(const cv::Mat &source, const cv::Mat &loadedMask) {
	if (source.empty())
		return cv::Mat();

	cv::Mat mask;

	// Use loaded PC_ mask if available
	// PC_ masks on disk: black (0) = valid, white (255) = masked
	// Internal representation: 255 = valid, 0 = masked
	if (!loadedMask.empty()) {
		// Invert the mask: black → 255, white → 0
		cv::bitwise_not(loadedMask, mask);
	} else {
		// Fallback: detect black pixels in source image
		mask = cv::Mat(source.rows, source.cols, CV_8UC1);
		for (int y = 0; y < source.rows; ++y) {
			const cv::Vec3b *srcRow = source.ptr<cv::Vec3b>(y);
			uchar *maskRow = mask.ptr<uchar>(y);
			for (int x = 0; x < source.cols; ++x) {
				const cv::Vec3b &pixel = srcRow[x];
				// Black pixel (B=0, G=0, R=0) → masked (0), otherwise valid (255)
				maskRow[x] = (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0) ? 0 : 255;
			}
		}
	}

	return mask;
}
