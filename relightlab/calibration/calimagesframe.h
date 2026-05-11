#ifndef CALIMAGESFRAME_H
#define CALIMAGESFRAME_H

#include "../imageframe.h"

class CalibrationSession;

// ImageFrame subclass for raw/calibration images.
// Overrides showImage, init, nextImage, previousImage, updateSkipped
// to operate on a CalibrationSession instead of the active Project.
class CalImagesFrame: public ImageFrame {
	Q_OBJECT
public:
	explicit CalImagesFrame(CalibrationSession *session, QWidget *parent = nullptr);

	void showImage(int id) override;
	void nextImage() override;
	void previousImage() override;
	void updateSkipped(int n) override;

public slots:
	void addImages();
	void removeSelected();

private:
	CalibrationSession *session;
};

#endif // CALIMAGESFRAME_H
