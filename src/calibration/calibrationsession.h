#ifndef CALIBRATIONSESSION_H
#define CALIBRATIONSESSION_H

#include "../image.h"
#include "../dome.h"
#include "calibration.h"

#include <QStringList>

/* CalibrationSession – data model for dome calibration.
 *
 * Implements ImageProvider so that standard display widgets (ImageFrame,
 * ImageView, ImageList, ImageGrid) work without code duplication.
 *
 * Holds:
 *   images       – list of source RAW / TIFF / JPEG paths
 *   dome         – dome geometry and light directions
 *   lens_calib   – Brown-Conrady distortion coefficients (from calibration.h)
 *   flatfield    – per-LED vignetting gain maps     (from calibration.h)
 *   white_point_percentile – luminance threshold for burned-pixel detection
 */
class CalibrationSession {
public:
	std::vector<Image>   images;
	Dome                 dome;
	LensCalibration      lens_calib;
	FlatfieldCalibration flatfield;
	double               white_point_percentile = 99.0;

	// ---- ImageProvider interface ----------------------------------------
	int          imageCount() const { return (int)images.size(); }
	Image       &imageAt(int i) { return images[i]; }
	const Image &imageAt(int i) const { return images[i]; }
	QImage       readImage(int i);

	// ---- Session management ---------------------------------------------
	void addImages(const QStringList &paths);
	void removeImage(int i);

	bool loadLP(const QString &path);
	bool saveLP(const QString &path) const;

	bool loadJson(const QString &path);
	bool saveJson(const QString &path) const;
};

#endif // CALIBRATIONSESSION_H
