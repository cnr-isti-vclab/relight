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
	QRectF boundingRect() const override;
	QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
signals:
	void itemChanged();
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

signals:
	void areaChanged(Crop crop);

public:
	//QRectF imageCroppedRect(); //return cropped rect in image ccords
	//QRect croppedRect();
	void enforceBounds(QRect rect, CursorPosition position); //makes sure the rectangle is inside the image.
	QRect ensureCropFits(QRect rect);

protected:
	virtual void update();
	virtual void resizeEvent(QResizeEvent *event);
	//virtual void paintEvent(QPaintEvent* _event);
	//virtual void mousePressEvent(QMouseEvent* _event);
	//virtual void mouseMoveEvent(QMouseEvent* _event);
	//virtual void mouseReleaseEvent(QMouseEvent* _event);

private:
	//CursorPosition cursorPosition(const QRectF& _cropRect, const QPointF& _mousePosition);
	void cornerMoved(int i);
	void updateCursorIcon(const QPointF& _mousePosition);
	void updateCrop();
	//void updateDeltaAndScale();

	//TODO invert logic: realSize rect is stored, cropping rect is computed instead.
//	float leftDelta = 0.0f, topDelta = 0.0f;
//	float xScale = 1.0f, yScale = 1.0f;

	QGraphicsRectItem *boundary = nullptr;
	CornerMarker *corners[9];
	QGraphicsLineItem *guide[2]; //horizontal, vertical

	bool isProportionFixed = false;
	QSizeF proportion = QSizeF(1.0, 1.0);

	//QRectF croppingRect; //in image coords //j


	//QRect realSizeRect;
	//QRect lastStaticCroppingRect;

	/*CursorPosition _cursorPosition = CursorPositionUndefined;
	//bool isMousePressed = false;

	QPointF startMousePos;

	QSizeF deltas = QSizeF(1.0, 1.0); */
	QColor backgroundColor = Qt::black;
	QColor borderColor = Qt::white;
	float handleMargin = 20;
};

#endif // IMAGECROPPER_H
