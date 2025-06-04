#ifndef IMAGECROPPER_H
#define IMAGECROPPER_H

#include <QWidget>
#include <QSizeF>
#include <QGraphicsRectItem>
#include "imageview.h"
#include "../src/crop.h"

enum CursorPosition {
	CursorPositionUndefined = 0,
	CursorPositionMiddle = 1 | 2 | 4 | 8,
	CursorPositionLeft = 1,
	CursorPositionTop = 2,
	CursorPositionRight = 4,
	CursorPositionBottom = 8,
	CursorPositionTopLeft = 1 | 2,
	CursorPositionTopRight = 2 | 4,
	CursorPositionBottomLeft = 8 | 1,
	CursorPositionBottomRight = 8 | 4
};

/* Selects a rectangular region (possibly rotated) on an image.
 * Coordinate system: First you rotate the image around the center of the image.
 * You get a new image (larger with black border), THEN you crop according to the crop rect.
 * If you rotate the image the crop rect stays where it is, eventually resized to ensure boundary are respected.
 * When exporting RTI, we need a function (static?) to compute the bounding box of the rotated crop,
 * so that we can export only that region, rotate and crop again (it will be the center!)
 */

class CornerMarker: public QObject, public QGraphicsRectItem {
	Q_OBJECT
public:
	bool silent = false; //stop itemChange to avoid loops.
	CornerMarker() {
		setCursor(Qt::CrossCursor);
		setFlag(QGraphicsItem::ItemIsMovable);
		setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
	}
	QRectF boundingRect() const override;
protected:
	QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
signals:
	void itemChanged();
};

class BoundaryMarker: public CornerMarker {
protected:
	QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
};

class ImageCropper : public ImageView {
	Q_OBJECT

public:
	ImageCropper(QWidget *parent = 0);
	~ImageCropper();
	Crop crop;

public slots:
	void setImage(const QPixmap& _image);
	void setBackgroundColor(const QColor& _backgroundColor);
	void setCroppingRectBorderColor(const QColor& _borderColor);

	void setProportion(const QSizeF& _proportion);
	void setProportionFixed(const bool _isFixed);

	void setCrop(Crop crop);
	void setRect(QRect rect);
	void setAngle(float a);

	void setWidth(int w);
	void setHeight(int h);
	void setTop(int t);
	void setLeft(int l);

	void resetCrop();
	void maximizeCrop();
	void centerCrop();

	QRect getRotatedSize();
	void updateScale();

signals:
	void areaChanged(Crop crop);

public:
	void enforceBounds(QRect rect, CursorPosition position); //makes sure the rectangle is inside the image.
	QRect ensureCropFits(QRect rect);

protected:
	virtual void update();
	virtual void resizeEvent(QResizeEvent *event);

private:
	void boundaryMoved();
	void cornerMoved(int i);
	void updateCursorIcon(const QPointF& _mousePosition);
	void updateCrop();

	BoundaryMarker *boundary = nullptr;
	CornerMarker *corners[9];
	QGraphicsLineItem *guide[2]; //horizontal, vertical

	bool isProportionFixed = false;
	QSizeF proportion = QSizeF(1.0, 1.0);

	double scale = 1.0f; //scale interface to look decent.
	QColor backgroundColor = Qt::black;
	QColor borderColor = Qt::white;
	float handleMargin = 20;
};

#endif // IMAGECROPPER_H
