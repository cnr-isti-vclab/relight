#ifndef GRIDCALIBVIEW_H
#define GRIDCALIBVIEW_H

#include "../canvas.h"

#include <QFrame>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsEllipseItem>
#include <vector>

/**
 * GridCalibView — a zoomable/pannable view for checkerboard calibration.
 *
 * Shows an image and overlays the detected inner-corner points.
 * Wraps Canvas (a QGraphicsView with smooth zoom) directly.
 */
class GridCalibView: public QFrame {
	Q_OBJECT
public:
	explicit GridCalibView(QWidget *parent = nullptr);

	/** Replace the displayed image.  Clears any existing corner overlay. */
	void setImage(const QPixmap &px);

	/** Overlay corner points on top of the current image.
	 *  @param pts  Corner positions in image-pixel coordinates.
	 *  @param cols Number of inner corners per row (used for colour-coding). */
	void setCorners(const std::vector<QPointF> &pts, int cols);

	/** Remove all corner overlay items. */
	void clearCorners();

	/** Fit the image in the available viewport. */
	void fit();

private:
	Canvas              *view    = nullptr;
	QGraphicsScene      *scene_  = nullptr;
	QGraphicsPixmapItem *pixItem = nullptr;

	std::vector<QGraphicsEllipseItem *> dotItems;

	static constexpr qreal DOT_R = 6.0;  // half-diameter in image pixels
};

#endif // GRIDCALIBVIEW_H
