#include "ortholoader.h"

#include <opencv2/core.hpp>
#include <opencv2/core/ocl.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/stitching/detail/blenders.hpp>
#include <opencv2/stitching/detail/seam_finders.hpp>

#include <QFileInfo>

#include <cassert>
#include <iostream>
#include <stdexcept>

using namespace std;

QImage bgrMatToQImage(const cv::Mat &bgr);
cv::Mat buildCoverageMask(const OrthoLoader::Tile &tile);
// RLE: alternating run-lengths of 0 and non-zero pixels, starting with the 0 run.
// e.g. {3, 5, 2} means 3 zeros, 5 ones, 2 zeros.
std::vector<uint32_t> compressMaskRLE(const cv::Mat &mask);
cv::Mat decompressMaskRLE(const std::vector<uint32_t> &rle, int rows, int cols);


int main(int argc, char *argv[]) try {
	cv::ocl::setUseOpenCL(false);  // évite CL_MEM_OBJECT_ALLOCATION_FAILURE sur gros orthos

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

	// Pass 1: build masks only (no BGR in RAM). Voronoi needs only sizes + corners + masks.
	std::vector<cv::Point> corners;
	std::vector<std::vector<uint32_t>> origMaskRLE;  // coverage mask before seam finding
	std::vector<std::vector<uint32_t>> seamMaskRLE;  // mask after seam finding

	{
		std::vector<cv::Size> sizes;
		std::vector<cv::UMat> masks;

		for (const OrthoLoader::Tile &tile : tiles) {
			cv::Mat mask = buildCoverageMask(tile);
			if (cv::countNonZero(mask) == 0)
				throw std::runtime_error("Tile has no valid (non-black) pixels: " + tile.imagePath.toStdString());

			sizes.push_back(mask.size());
			corners.push_back(cv::Point(tile.x, tile.y));
			origMaskRLE.push_back(compressMaskRLE(mask));
			masks.push_back(mask.getUMat(cv::ACCESS_RW));
		}

		if (masks.empty()) {
			cerr << "Blending failed: No valid pixels were submitted to the blender." << endl;
			return 1;
		}

		// Voronoi seam finder: geometry only (sizes+corners+masks), no image content → saves RAM
		cv::detail::VoronoiSeamFinder seam_finder;
		seam_finder.find(sizes, corners, masks);

		// Compress seam-adjusted masks
		seamMaskRLE.reserve(masks.size());
		for (cv::UMat &um : masks) {
			cv::Mat m = um.getMat(cv::ACCESS_READ);
			seamMaskRLE.push_back(compressMaskRLE(m));
		}
	}

	const cv::Rect roi(0, 0, canvasSize.width(), canvasSize.height());
	cv::detail::MultiBandBlender blender(false, numBands);
	blender.prepare(roi);

	// Pass 2: load one tile at a time, fill, feed, unload
	for (int i = 0; i < tiles.size(); ++i) {
		const OrthoLoader::Tile &tile = tiles[i];
		cv::Mat bgr = cv::imread(tile.imagePath.toStdString(), cv::IMREAD_COLOR);
		if (bgr.empty())
			throw std::runtime_error("Failed to load tile image: " + tile.imagePath.toStdString());
		cv::Mat origMask = decompressMaskRLE(origMaskRLE[(size_t)i], bgr.rows, bgr.cols);
		cv::Mat seamMask = decompressMaskRLE(seamMaskRLE[(size_t)i], bgr.rows, bgr.cols);

		//use the original mask to minimize bleeding.s
		cv::Scalar avgColor = cv::mean(bgr, origMask);
		cv::Mat zeroMask = (origMask == 0);
		bgr.setTo(avgColor, zeroMask);

		cv::Mat img16s;
		bgr.convertTo(img16s, CV_16SC3);
		blender.feed(img16s, seamMask, corners[i]);
		// bgr released at end of iteration
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
} catch (const std::exception &e) {
	cerr << "Error: " << e.what() << endl;
	return 1;
}


QImage bgrMatToQImage(const cv::Mat &bgr) {
	assert(!bgr.empty() && bgr.type() == CV_8UC3);

	cv::Mat rgb;
	cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
	QImage image(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
	return image.copy();
}

// RLE: alternating run-lengths of 0-pixels then non-zero pixels, first run is always 0-pixels.
std::vector<uint32_t> compressMaskRLE(const cv::Mat &mask) {
	assert(!mask.empty() && mask.type() == CV_8UC1 && mask.isContinuous());
	std::vector<uint32_t> rle;
	const size_t n = (size_t)mask.rows * mask.cols;
	const uchar *data = mask.data;
	bool current = false; // first run is always the 0-pixel run
	uint32_t count = 0;
	for (size_t i = 0; i < n; ++i) {
		bool v = data[i] != 0;
		if (v == current) {
			++count;
		} else {
			rle.push_back(count);
			count = 1;
			current = v;
		}
	}
	rle.push_back(count);
	return rle;
}

cv::Mat decompressMaskRLE(const std::vector<uint32_t> &rle, int rows, int cols) {
	cv::Mat mask(rows, cols, CV_8UC1);
	assert(mask.isContinuous());
	uchar *data = mask.data;
	size_t pos = 0;
	const size_t total = (size_t)rows * cols;
	bool current = false; // first run is always the 0-pixel run
	for (uint32_t runLen : rle) {
		const uchar val = current ? 255 : 0;
		assert(pos + runLen <= total);
		memset(data + pos, val, runLen);
		pos += runLen;
		current = !current;
	}
	assert(pos == total);
	return mask;
}

// PC_ masks on disk: black (0) = valid, white (255) = masked.
// Internal representation: 255 = valid, 0 = masked.
cv::Mat buildCoverageMask(const OrthoLoader::Tile &tile) {
	if (!tile.maskPath.isEmpty() && QFileInfo::exists(tile.maskPath)) {
		cv::Mat pcMask = cv::imread(tile.maskPath.toStdString(), cv::IMREAD_GRAYSCALE);
		if (pcMask.empty())
			throw std::runtime_error("Failed to load mask file: " + tile.maskPath.toStdString());
		cv::Mat mask;
		cv::bitwise_not(pcMask, mask);  // black=valid → 255, white=masked → 0
		return mask;
	}

	// Fallback: detect black pixels in source image.
	cv::Mat bgr = cv::imread(tile.imagePath.toStdString(), cv::IMREAD_COLOR);
	if (bgr.empty())
		throw std::runtime_error("Failed to load tile image: " + tile.imagePath.toStdString());
	cv::Mat mask(bgr.rows, bgr.cols, CV_8UC1);
	for (int y = 0; y < bgr.rows; ++y) {
		const cv::Vec3b *srcRow = bgr.ptr<cv::Vec3b>(y);
		uchar *maskRow = mask.ptr<uchar>(y);
		for (int x = 0; x < bgr.cols; ++x) {
			const cv::Vec3b &pixel = srcRow[x];
			// Black pixel (B=0, G=0, R=0) → masked (0), otherwise valid (255).
			maskRow[x] = (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0) ? 0 : 255;
		}
	}
	return mask;
}
