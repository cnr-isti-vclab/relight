#ifndef RELIGHT_AUTOALIGN_H
#define RELIGHT_AUTOALIGN_H

#include <vector>
#include <functional>
#include <optional>
#include <complex>
#include <cstdint>

#include <Eigen/Core>
#include <QPointF>
#include <QRect>
#include <QSize>
#include <QString>

class QImage;

struct AutoAlignPeak {
	Eigen::Vector2f offset; //relative position of the second image with respect to the first
	float response; //phase-correlation peak value
};

struct AutoAlignCorrespondence {
	int first; //provided-image index
	int second; //provided-image index, greater than first
	std::optional<AutoAlignPeak> peak; //Keeping multiple peaks might help resolve ambiguities.
};

/* run() automatically chooses textured regions and jointly solves their correlations.
	beginRegion()/provideImage()/findCandidates() analyze an even square region of at least 40 pixels. */

class AutoAlign {
public:
	const int max_side = 1536; //images are downscaled to this size.

	// run() calls the loader concurrently; implementations must be thread-safe.
	typedef std::function<QImage(int i)> Loader;

	AutoAlign(QSize image_size);
	//pixel (x, y) of the first image is at (x, y) + offset in image i. Throws QString on failure or cancel.
	std::vector<QPointF> run(int n, Loader load, std::function<bool(QString stage, int percent)> *callback = nullptr);

	//Returns the strongest phase-correlation peak for each edge i -> i + 2^k.
	void beginRegion(QSize region_size, int image_count);
	void provideImage(int image_index, const uint8_t *image, int channels);

	std::vector<AutoAlignCorrespondence> findCandidates(
		std::function<bool(QString stage, int percent)> *callback = nullptr
	);
	//Solves image translations from the strongest candidate on each edge, anchoring image 0 at zero.
	std::vector<Eigen::Vector2f> solveTranslations(
		int n,
		const std::vector<AutoAlignCorrespondence> &correspondences
	) const;


private:
	int w = 0, h = 0; //downscaled size
	int side = 400; //tile size, in downscaled pixels
	int region_side = 0;
	int region_image_count = 0;
	std::vector<float> region_window;
	std::vector<std::vector<std::complex<float>>> region_spectra;
};

#endif // RELIGHT_AUTOALIGN_H
