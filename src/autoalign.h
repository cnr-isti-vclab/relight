#ifndef AUTOALIGN_H
#define AUTOALIGN_H

#include <vector>
#include <functional>

#include <QPointF>
#include <QRect>
#include <QSize>
#include <QString>

class QImage;

struct AutoAlignPeak {
	QPointF offset; //relative position of the second image with respect to the first
	float response; //phase-correlation peak value
};

struct AutoAlignCorrespondence {
	int first; //loader index
	int second; //loader index, greater than first
	std::vector<AutoAlignPeak> peaks;
};

/* run() retains the legacy automatic tile consensus. runRegion() analyzes a user-selected region
   and returns candidate correspondences without selecting a final alignment. */

class AutoAlign {
public:
	const int max_side = 1536; //images are downscaled to this size.

	//returns image i downscaled to size.
	typedef std::function<QImage(int i, QSize size)> Loader;
	typedef std::function<QImage(int i, QRect region)> RegionLoader;

	AutoAlign(QSize image_size);
	//pixel (x, y) of the first image is at (x, y) + offset in image i. Throws QString on failure or cancel.
	std::vector<QPointF> run(int n, Loader load, std::function<bool(QString stage, int percent)> *callback = nullptr);
	//Returns up to four phase-correlation peaks for each edge i -> i + 2^k.
	//The graph indices refer to loader indices; peak offsets are full-resolution.
	std::vector<AutoAlignCorrespondence> runRegion(
		QRect region,
		int n,
		RegionLoader load,
		std::function<bool(QString stage, int percent)> *callback = nullptr
	);
	//Solves image translations from the strongest candidate on each edge, anchoring image 0 at zero.
	std::vector<QPointF> solveTranslations(
		int n,
		const std::vector<AutoAlignCorrespondence> &correspondences
	) const;

	QRect region; //full resolution area of the tile that matched best, after run().

private:
	QSize image_size;
	int w = 0, h = 0; //downscaled size
	float scale = 1.0f; //full size / downscaled size
	int side = 256; //tile size, in downscaled pixels
};

#endif // AUTOALIGN_H
