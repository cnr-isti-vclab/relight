#ifndef IMAGECROPPER_H
#define IMAGECROPPER_H

#include <QWidget>
#include <QSizeF>

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

class ImageCropper : public QWidget {
	Q_OBJECT

public:
	ImageCropper(QWidget *parent = 0);
	~ImageCropper();

public slots:
	void setImage(const QPixmap& _image);
	void setCroppingRectBorderColor(const QColor& _borderColor);

	void setProportion(const QSizeF& _proportion);
	void setProportionFixed(const bool _isFixed);

	void showHandle(bool _show = true);
	void hideHandle();

	void setCrop(QRect rect);
	void setWidth(int w);
	void setHeight(int h);
	void setTop(int t);
	void setLeft(int l);
	void resetCrop();
	void maximizeCrop();
	void centerCrop();

signals:
	void areaChanged(QRect rect);

public:
	bool handleShown() { return show_handle; }
	QRectF imageCroppedRect(); //return cropped rect in image ccords
	QRect croppedRect();
	void enforceBounds(QRect rect, CursorPosition position); //makes sure the rectangle is inside the image.
	QRect ensureCropFits(QRect rect);
protected:
	virtual void resizeEvent(QResizeEvent *event);
	virtual void paintEvent(QPaintEvent* _event);
	virtual void mousePressEvent(QMouseEvent* _event);
	virtual void mouseMoveEvent(QMouseEvent* _event);
	virtual void mouseReleaseEvent(QMouseEvent* _event);

private:
	CursorPosition cursorPosition(const QRectF& _cropRect, const QPointF& _mousePosition);
	void updateCursorIcon(const QPointF& _mousePosition);
	void updateDeltaAndScale();


private:
	bool show_handle = true;

	//TODO invert logic: realSize rect is stored, cropping rect is computed instead.
	float leftDelta = 0, topDelta = 0;
	float xScale = 1, yScale = 1;

	QPixmap image;
	//QRectF croppingRect; //in image coords //j

	QRect realSizeRect;
	QRect lastStaticCroppingRect;

	CursorPosition _cursorPosition = CursorPositionUndefined;
	bool isMousePressed = false;
	bool isProportionFixed = false;
	QPointF startMousePos;
	QSizeF proportion = QSizeF(1.0, 1.0);
	QSizeF deltas = QSizeF(1.0, 1.0);
	QColor backgroundColor = Qt::black;
	QColor croppingRectBorderColor = Qt::white;
	float handleMargin = 20;

};

#endif // IMAGECROPPER_H
